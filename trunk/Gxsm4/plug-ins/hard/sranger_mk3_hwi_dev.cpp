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

/* irnore this module for docuscan
% PlugInModuleIgnore
 */

#define _FILE_OFFSET_BITS 64

#include <locale.h>
#include <libintl.h>
#include <time.h>

#include <climits>    // for limits macrors
#include <iomanip>    // for setfill, setprecision, setw
#include <ios>        // for dec, hex, oct, showbase, showpos, etc.
#include <iostream>   // for cout
#include <sstream>    // for istringstream

#include "glbvars.h"
#include "xsmtypes.h"
#include "xsmdebug.h"
#include "surface.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include "sranger_mk3_hwi.h"
#include "xsmcmd.h"

// need some SRanger io-controls 
// HAS TO BE IDENTICAL TO THE DRIVER's FILE!
// so far the same for mk2 and mk3
#include "modules/sranger_mk23_ioctl.h"

// SRanger data exchange structs and consts
#include "MK3-A810_spmcontrol/FB_spm_dataexchange.h" 

#ifdef __SRANGER_MK3_DSP_SIGNALS_H
#undef __SRANGER_MK3_DSP_SIGNALS_H
#endif
#define CREATE_DSP_SIGNAL_LOOKUP
#include "MK3-A810_spmcontrol/dsp_signals.h"  


// enable debug:
#define	SRANGER_DEBUG(S) PI_DEBUG (DBG_L4, S)
#define	SRANGER_DEBUG_SIG(S) PI_DEBUG (DBG_L1, S)
#define	SRANGER_DEBUG_GP(ARGS...)  PI_DEBUG_GP (DBG_L2, ARGS)
// #define	PI_DEBUG_GP(DBG_L, ARGS...)  if(pi_debug_level >= DBG_L) g_message (ARGS)
#define	SRANGER_ERROR(S) PI_DEBUG_ERROR (DBG_L2, S)

#define SR_READFIFO_RESET -1
#define SR_EMPTY_PROBE_FIFO -2

extern int developer_option;
extern int pi_debug_level;

extern DSPControl *DSPControlClass;
extern GxsmPlugin sranger_mk2_hwi_pi;
extern DSPPACControl *DSPPACClass;

// some thread states

#define RET_FR_OK      0
#define RET_FR_ERROR   -1
#define RET_FR_WAIT    1
#define RET_FR_NOWAIT  2
#define RET_FR_FCT_END 3

#define FR_NO   0
#define FR_YES  1

#define FR_INIT   1
#define FR_FINISH 2
#define FR_FIFO_FORCE_RESET 3

#define CONST_DSP_F16 65536.
#define VOLT2AIC(U)   (int)(main_get_gapp()->xsm->Inst->VoltOut2Dig (main_get_gapp()->xsm->Inst->BiasV2V (U)))
#define DVOLT2AIC(U)  (int)(main_get_gapp()->xsm->Inst->VoltOut2Dig ((U)/main_get_gapp()->xsm->Inst->BiasGainV2V ()))


#define CPN(N) ((double)(1LL<<(N))-1.)


gpointer FifoReadThread3 (void *ptr_sr);
gpointer ProbeFifoReadThread3 (void *ptr_sr);
gpointer ProbeFifoReadFunction3 (void *ptr_sr, int dspdev);



/* Construktor for Gxsm sranger support base class
 * ==================================================
 * - open device
 * - init things
 * - ...
 */
sranger_mk3_hwi_dev::sranger_mk3_hwi_dev(){
        gchar *tmp;
	g_message ("HWI-DEV-MK3-I** HwI SR-MK3: verifying MK3 FB_SPM software details.");
	PI_DEBUG_GP (DBG_L1, " -> HWI-DEV-MK3-I** HwI SR-MK3: verifying MK3 FB_SPM software details.\n");
        main_get_gapp()->monitorcontrol->LogEvent ("HWI-DEV-MK3-I** HwI SR-MK3", "verifying MK3 FB_SPM software details.");
	AIC_max_points = 1<<15; // SR-AIC resolution is limiting...
	fifo_read_thread = NULL;
	probe_fifo_read_thread = NULL;
	probe_fifo_thread_active = FALSE;
	thread_dsp = 0;
	productid = g_strdup ("not yet identified");
	swap_flg = 0;
	magic_data.magic = 0; // set to zero, this means all data is invalid!
	dsp_alternative = 0;
	target = SR_HWI_TARGET_NONE;
	sranger_mark_id = -2;

	// just this as reference is set to NULL as indicatior all pll refs need to be read from DSP first.
	dsp_pll.volumeSine = 0;

	dsp=0;
	thread_dsp=0;
	probe_thread_dsp=0;
	dsp_alternative=0;

	bz_statistics[0]=0;
	bz_statistics[1]=0;
	bz_statistics[2]=0;
	bz_statistics[3]=0;
	bz_statistics[4]=0;

	gint srdev_index_start = atoi ( strrchr (xsmres.DSPDev, '_')+1); // start at given device, keep looking for higher numbers
	while (srdev_index_start >= 0 && srdev_index_start < 8) {
                if (!strcmp (xsmres.DSPDev, "/dev/shm/sranger_mk3emu"))
                        srdev_index_start = 8; // no more, try just this
                else
                        sprintf (xsmres.DSPDev,"/dev/sranger_mk2_%d", srdev_index_start); // override
                PI_DEBUG_GP (DBG_L1, " -> Looking for MK3 up and running GXSM compatible FB_spmcontrol DSP code starting at %s.\n", xsmres.DSPDev);
	  
		if((dsp = open (xsmres.DSPDev, O_RDWR)) <= 0){
		        PI_DEBUG_GP (DBG_L1, " -> HWI-DEV-MK3 E01-- can not open device >%s<, please check permissions. \nError: %s\n", 
                                     xsmres.DSPDev, strerror(errno));
			dsp = 0;
			if (++srdev_index_start < 8){
				PI_DEBUG_GP (DBG_L1, " -> HWI-DEV-MK3 MSG01 -- Continue searching for next device...\n");
				continue;
			}
			SRANGER_ERROR(
				      "Can´t open <" << xsmres.DSPDev << ">, reason: " << strerror(errno) << std::endl
				      << "please make sure:" << std::endl
				      << "-> that the device exists and has proper access rights" << std::endl
				      << "-> that the kernel module loaded" << std::endl
				      << "-> the USB connection to SignalRanger" << std::endl
				      << "-> that the SR-MK2/3 is powered on");
			productid=g_strdup_printf ("Device used: %s\n\n"
						   "Make sure:\n"
						   "-> that the device exists and has proper access rights\n"
						   "-> that the kernel module loaded (try 'dmesg')\n"
						   "-> the USB connection to SignalRanger is OK\n"
						   "-> that the SR-MK2/3 is powered on\n"
						   "Start 'gxsm4 -h no' to change the device path/name.",
						   xsmres.DSPDev);
			main_get_gapp()->alert (N_("No Hardware"), N_("Open Device failed."), productid, 1);
                        main_get_gapp()->monitorcontrol->LogEvent (N_("No Hardware"), N_("Open Device failed."));
			exit (-1);
			return;
		}
		else{
		        int ret;
			unsigned int vendor=0, product=0;
                        if (!strcmp (xsmres.DSPDev, "/dev/shm/sranger_mk3emu")){
                                vendor  = 0x1612;
                                product = 0xf103;
                        } else {
                                if ((ret=ioctl(dsp, SRANGER_MK2_IOCTL_VENDOR, (unsigned long)&vendor)) < 0){
                                        SRANGER_ERROR(strerror(ret) << " cannot query VENDOR" << std::endl
                                                      << "Device: " << xsmres.DSPDev);
                                        g_free (productid);
                                        productid=g_strdup_printf ("Device used: %s\n Start 'gxsm4 -h no' to correct the problem.", xsmres.DSPDev);
                                        main_get_gapp()->alert (N_("Unkonwn Hardware"), N_("Query Vendor ID failed."), productid, 1);
                                        PI_DEBUG_GP (DBG_L4, "HWI-DEV-MK3-E02-- unkown hardware, vendor ID mismatch.\n");
                                        close (dsp);
                                        dsp = 0;
                                        exit (-1);
                                        return;
                                }
                                if ((ret=ioctl(dsp, SRANGER_MK2_IOCTL_PRODUCT, (unsigned long)&product)) < 0){
                                        SRANGER_ERROR(strerror(ret) << " cannot query PRODUCT" << std::endl
                                                      << "Device: " << xsmres.DSPDev);
                                        g_free (productid);
                                        productid=g_strdup_printf ("Device used: %s\n Start 'gxsm4 -h no' to correct the problem.", xsmres.DSPDev);
                                        main_get_gapp()->alert (N_("Unkonwn Hardware"), N_("Query Product ID failed."), productid, 1);
                                        PI_DEBUG_GP (DBG_L4, "HWI-DEV-MK3-E03-- unkown hardware, product ID mismatch.\n");
                                        close (dsp);
                                        dsp = 0;
                                        return;
                                }
                        }
			if (vendor == 0x0a59 || vendor == 0x1612){
			        g_free (productid);
				if (vendor == 0x0a59 && product == 0x0101){
				        productid=g_strdup ("Vendor/Product: B.Paillard, Signal Ranger STD");
					main_get_gapp()->alert (N_("Wrong Hardware detected"), N_("No MK2 found."), productid, 1);
					PI_DEBUG_GP (DBG_L4, "HWI-DEV-MK3-E04-- wrong hardware: SR-STD, please use SR-SP2/STD HwI.\n");
					target = SR_HWI_TARGET_C54;
					close (dsp);
					dsp=0;
					exit (-1);
					return;
				}
				else if (vendor == 0x0a59 && product == 0x0103){
				        productid=g_strdup ("Vendor/Product: B.Paillard, Signal Ranger SP2");
					main_get_gapp()-> alert (N_("Wrong Hardware detected"), N_("No MK2 found."), productid, 1);
					PI_DEBUG_GP (DBG_L4, "HWI-DEV-MK3-E05-- wrong hardware: SR-SP2, please use SR-SP2/STD HwI.\n");
					target =  SR_HWI_TARGET_C54;
					close (dsp);
					dsp=0;
					exit (-1);
					return;
				}else if (vendor == 0x1612 && product == 0x0100){
				        productid=g_strdup ("Vendor/Product: B.Paillard, Signal Ranger MK2 (1612:0100)");
					PI_DEBUG_GP (DBG_L4, "HWI-DEV-MK3-I01-- SR-MK2 1612:0100 found (autoprobe continue).\n");
					target = SR_HWI_TARGET_C55;
					close (dsp);
					dsp=0;
					return;
				}else if (vendor == 0x1612 && product == 0x0101){
				        productid=g_strdup ("Vendor/Product: B.Paillard, Signal Ranger MK2-NG (1612:0101)");
					PI_DEBUG_GP (DBG_L4, "HWI-DEV-MK3-I02-- SR-MK2-NG 1612:0101 found (autoprobe continue).\n");
					target = SR_HWI_TARGET_C55;
					close (dsp);
					dsp=0;
					return;
				}else if (vendor == 0x1612 && product == 0x0103){
				        productid=g_strdup ("Vendor/Product: B.Paillard, Signal Ranger MK3 (1612:0103)");
					PI_DEBUG_GP (DBG_L4, "HWI-DEV-MK3-I03-- SR-MK3-NG 1612:0103 found -- OK, taking it.\n");
					target = SR_HWI_TARGET_C55;
					sranger_mark_id = 3;
				}else if (vendor == 0x1612 && product == 0xf103){
				        productid=g_strdup ("Vendor/Product: GXSM4-EMU, Signal Ranger MK3 EMU (1612:F103)");
					PI_DEBUG_GP (DBG_L4, "HWI-DEV-MK3-SHMEMU-I03-- SR-MK3-NG-EMU 1612:F103 found -- OK, taking it.\n");
					target = SR_HWI_TARGET_C55;
					sranger_mark_id = 3;
				}else{
				        productid=g_strdup ("Vendor/Product: B.Paillard, unkown version!");
					main_get_gapp()->alert (N_("Unkonwn Hardware detected"), N_("No Signal Ranger found."), productid, 1);
					PI_DEBUG_GP (DBG_L4, "HWI-DEV-MK3-E06-- SR-MK?? 1612:%s found -- not supported.\n", productid);
					close (dsp);
					dsp=0;
					return;
				}
				
				PI_DEBUG_GP (DBG_L4, "HWI-DEV-MK3-I** reading DSP magic informations and code version.\n");


				// now read magic struct data
                                if (product == 0xf103){ // shm dsp emu (mmapped emulator)
                                        main_get_gapp()->monitorcontrol->LogEvent ("HWI-DEV-MK3 DSP-ENGINE","MMAPPED DSP EMULATOR IDENTIFIED");
                                        PI_DEBUG_GP (DBG_L4, "HWI-DEV-MK3 DSP-ENGINE: MMAPPED DSP EMULATOR IDENTIFIED");
                                        // DSP EMU MAGIC IS MAPPED TO BEGINNING (adr=0) OF DSP MMAPPER BUFFER 
                                        lseek (dsp, 0, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
				} else {
                                        main_get_gapp()->monitorcontrol->LogEvent ("HWI-DEV-MK3 DSP-ENGINE","USB DEVICE MAPPED TI DSP");
                                        PI_DEBUG_GP (DBG_L4, "HWI-DEV-MK3 DSP-ENGINE: USB DEVICE MAPPED TI DSP");
                                        lseek (dsp, FB_SPM_MAGIC_ADR, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
                                }
                                
                                // now read magic struct data
				//lseek (dsp, FB_SPM_MAGIC_ADR, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
				sr_read (dsp, &magic_data, sizeof (magic_data)); 
				swap_flg = 0;
				PI_DEBUG_GP (DBG_L4, "HWI-DEV-MK3-I*CPU* DSP-MAGIC  : %08x   [M:%08x]\n", magic_data.magic, FB_SPM_MAGIC);
				if (magic_data.magic != FB_SPM_MAGIC){
				        close (dsp);
					dsp=0;
					SRANGER_DEBUG ("Wrong Magic");
					PI_DEBUG_GP (DBG_L4, "HWI-DEV-MK3-I** weird endianess of host (PPC?) not support yet.\n");

					++srdev_index_start;
					if (srdev_index_start < 8)
					        continue; // auto attempt to try next board if any
					exit (-1);
					return;
				}
				tmp = g_strdup_printf ("%08x   [HwI:%08x]", magic_data.dsp_soft_id, FB_SPM_SOFT_ID);
                                main_get_gapp()->monitorcontrol->LogEvent ("HWI-DEV-MK3-I*CPU* DSP-SoftId.... ", tmp);
				PI_DEBUG_GP (DBG_L4, "HWI-DEV-MK3-I*CPU* DSP-SoftId...: %s\n", tmp); g_free (tmp);

				tmp = g_strdup_printf ("%08x   [HwI:%08x]", magic_data.version, FB_SPM_VERSION);
                                main_get_gapp()->monitorcontrol->LogEvent ("HWI-DEV-MK3-I*CPU* DSP-Version... ", tmp);
				PI_DEBUG_GP (DBG_L4, "HWI-DEV-MK3-I*CPU* DSP-Version..: %s\n", tmp); g_free (tmp);

				tmp = g_strdup_printf ("0x%08x", magic_data.signal_lookup);
				main_get_gapp()->monitorcontrol->LogEvent ("HWI-DEV-MK3-I*CPU* DSP-Magic::SignalLookup at address", tmp);
				PI_DEBUG_GP (DBG_L4, "HWI-DEV-MK3-I*CPU* DSP-Magic::SignalLookup: %s\n", tmp); g_free (tmp);
                                
				if (FB_SPM_VERSION != magic_data.version || 
				    FB_SPM_SOFT_ID != magic_data.dsp_soft_id){
				        gchar *details = g_strdup_printf("Detected SRanger DSP Software Version: %x.%02x\n"
									 "GXSM was build for DSP Software Version: %x.%02x\n\n"
									 "Note: This may cause incompatility problems and unpredictable toubles,\n"
									 "however, trying to proceed in case you know what you are doing is possible.\n\n"
									 "Please update the MK3 DSP (reprogramm the flash) or match GXSM software.\n\n"
									 "SwapFlag: %d   Target: %d\n",
									 magic_data.version >> 8, 
									 magic_data.version & 0xff,
									 FB_SPM_VERSION >> 8, 
									 FB_SPM_VERSION & 0xff,
									 swap_flg, target);
                                        SRANGER_DEBUG ("Signal Ranger FB_SPM soft Version mismatch\n" << details);
					PI_DEBUG_GP (DBG_L4, "HWI-DEV-MK3-VW01-- DSP software version mismatch warning.\n%s\n", details);
					main_get_gapp()->alert (N_("Critical Warning:"), N_("Signal Ranger MK3 FB_SPM software version mismatch detected! Exiting required."), details,
                                                     developer_option == 0 ? 20 : 5);
                                        main_get_gapp()->monitorcontrol->LogEvent ("Critical: Signal Ranger MK3 FB_SPM soft Version mismatch:\n", details);
                                        g_critical ("Signal Ranger MK3 FB_SPM soft Version mismatch:\n%s", details);
					g_free (details);

                                        //if (developer_option == 0)
                                        //        exit (-1);
                                        
				}
				
				SRANGER_DEBUG ("ProductId:" << productid);

				{
                                        gchar *InfoString = g_strdup_printf("\nSR-MK3-PLL connected at %s"
                                                                            "\nDSP-SoftId : %04x [HwI:%04x]"
                                                                            "\nDSP-SoftVer: %04x [HwI:%04x]"
                                                                            "\nDSP-SoftDat: %04x %04x [HwI:%04x %04x]",
                                                                            xsmres.DSPDev,
                                                                            magic_data.dsp_soft_id, FB_SPM_SOFT_ID,
                                                                            magic_data.version, FB_SPM_VERSION,
                                                                            magic_data.mmdd, magic_data.year, FB_SPM_DATE_MMDD, FB_SPM_DATE_YEAR
                                                                            );
                                        
                                        main_get_gapp()->monitorcontrol->LogEvent ("MK3 HwI Intialization and DSP software verification completed", InfoString);
                                        g_free (InfoString);
                                }
                                
				// open some more DSP connections, used by threads
				if((thread_dsp = open (xsmres.DSPDev, O_RDWR)) <= 0){
				        SRANGER_ERROR ("cannot open thread SR connection, trying to continue...");
					PI_DEBUG_GP (DBG_L4, "HWI-DEV-MK3-OTE01-- open multi dev error, can not generate thread DSP link.\n");
					thread_dsp = 0;
				}
				// testing...
				if((probe_thread_dsp = open (xsmres.DSPDev, O_RDWR)) <= 0){
				        SRANGER_ERROR ("cannot open probe thread SR connection, trying to continue...");
					PI_DEBUG_GP (DBG_L4, "HWI-DEV-MK3-OTE02-- open multi dev error, can not generate probe DSP link.\n");
					probe_thread_dsp = 0;
				}
				if((dsp_alternative = open (xsmres.DSPDev, O_RDWR)) <= 0){
				        SRANGER_ERROR ("cannot open alternative SR connection, trying to continue...");
					PI_DEBUG_GP (DBG_L4, "HWI-DEV-MK3-OTE03-- open multi dev error, can not generate alternate DSP link.\n");
					dsp_alternative = 0;
				}
			}else{
			        SRANGER_ERROR ("unkown vendor, exiting");
				PI_DEBUG_GP (DBG_L4, "HWI-DEV-MK3-E07-- Unkown Device Vendor ID.\n");
				close (dsp);
				dsp = 0;
				return;
			}
		}
		break;
	}

// verify / autocorrect and notify about channel configurations needed for MK2-A810 in HR mode
	{	
		int f=0;
		GString *details = g_string_new ("\n==> Auto Adjustments done:\n\n");
		for (int i=0; i<PIDCHMAX; ++i)
			if (strncmp (xsmres.pidsrcZtype[i], "FLOAT", 5)){
				g_string_append_printf (details, "DataAq/PIDSrcA%d-ZType=%s -> FLOAT   [MK2-%s]\n", i+1, xsmres.pidsrcZtype[i], i==0?"Z_HR":"N/A");
				strcpy (xsmres.pidsrcZtype[i], "FLOAT");
				f=1;
			}
		for (int i=0; i<8; ++i)
			if (strncmp (xsmres.daqZtype[i], "FLOAT", 5)){
				g_string_append_printf (details, "DataAq/DataSrc%s%d-ZType=%s -> FLOAT  [A810-ADC%d]\n", i<4?"A":"B", i%4+1, xsmres.daqZtype[i], i);
				strcpy (xsmres.daqZtype[i], "FLOAT");
				f=1;
			}
		for (int i=8; i<DAQCHMAX; ++i)
			if (strncmp (xsmres.daqZtype[i], "FLOAT", 4)){
				g_string_append_printf (details, "DataAq/DataSrc%c1-ZType=%s -> FLOAT   [MK2-Data%d]\n", 'C'+i-8, xsmres.daqZtype[i], i);
				strcpy (xsmres.daqZtype[i], "LONG");
				f=1;
			}

		// -- have to do this here for now as HwI not yet loaded --
		// MUST HAVE ALL CHANNELS ENABLED
		for(int k=1; k<PIDCHMAX; k++)
			if(!strcmp(xsmres.pidsrc[k], "-")){
				strcpy (xsmres.pidsrc[k],"MIX"), xsmres.pidsrc[k][3]='0'+k;
				g_string_append_printf (details, "DataAq/DataPIDSrc[%d]=%s -> [must be enabled, auto mapped with MK3 Signals]\n", k ,xsmres.pidsrc[k]);
				f=1;
			}
		
		for(int k=0; k<DAQCHMAX; k++)
			if(!strcmp(xsmres.daqsrc[k], "-")){
				if (k<8)
					strcpy (xsmres.daqsrc[k],"AIC"), xsmres.daqsrc[k][3] = '0'+k;
				else
					strcpy (xsmres.daqsrc[k],"SIG"), xsmres.daqsrc[k][3] = 'a'+k-8;
				g_string_append_printf (details, "DataAq/DataAqSrc[%d]=%s -> [must be enabled, auto mapped with MK3 Signals]\n", k ,xsmres.daqsrc[k]);
				f=1;
			}

		if (f){
			g_string_append_printf (details, "\n\nPLEASE VERIFY YOUR PREFERENCES!");
			main_get_gapp()->alert (N_("Warning"), N_("MK3-A810/PLL DataAq Preferences Setup Verification"), details->str, 1);
			PI_DEBUG_GP (DBG_L4, "HWI-DEV-MK3-WW00A-- configuration/preferences mismatch with current hardware setup -- please adjust.\n");
		}
		g_string_free(details, TRUE); 
		details=NULL;
	}
}

/* Destructor
 * close device
 */
sranger_mk3_hwi_dev::~sranger_mk3_hwi_dev(){
	SRANGER_DEBUG ("closing connection to SRanger driver");
	if (dsp) close (dsp);
	if (thread_dsp) close (thread_dsp);
	if (probe_thread_dsp) close (probe_thread_dsp);
	if (dsp_alternative) close (dsp_alternative);
	if (productid) g_free (productid);
}

// data translation helpers

void sranger_mk3_hwi_dev::swap (guint16 *addr){
	guint16 temp1, temp2;
	temp1 = temp2 = *addr;
	*addr = ((temp2 & 0xFF) << 8) | ((temp1 >> 8) & 0xFF);
}

void sranger_mk3_hwi_dev::swap (gint16 *addr){
	guint16 temp1, temp2;
	temp1 = temp2 = *addr;
	*addr = ((temp2 & 0xFF) << 8) | ((temp1 >> 8) & 0xFF);
}

void sranger_mk3_hwi_dev::swap (gint32 *addr){
	gint32 temp1, temp2, temp3, temp4;

	temp1 = (*addr)       & 0xFF;
	temp2 = (*addr >> 8)  & 0xFF;
	temp3 = (*addr >> 16) & 0xFF;
	temp4 = (*addr >> 24) & 0xFF;


	if (target == SR_HWI_TARGET_C55)
		*addr = (temp3 << 24) | (temp4 << 16) | (temp1 << 8) | temp2;
	else // can assume so far else is: if (target == SR_HWI_TARGET_C54)
		*addr = (temp1 << 24) | (temp2 << 16) | (temp3 << 8) | temp4;
}

gint32 sranger_mk3_hwi_dev::float_2_sranger_q15 (double x){
	if (x >= 1.)
		return 32767;
	if (x <= -1.)
		return -32766;
	
	return (gint32)(x*32767.);
}

// 1<<31 = 2147483648
gint32 sranger_mk3_hwi_dev::float_2_sranger_q31 (double x){
	if (x >= 1.)
		return 0x7FFFFFFF;
	if (x <= -1.)
		return -0x7FFFFFFE;
	
	return (gint32)round(x*2147483647.);
}

gint32 sranger_mk3_hwi_dev::int_2_sranger_int (gint32 x){
	gint16 tmp = x > 32767 ? 32767 : x < -32766 ? -32766 : x; // saturate
#if 0
	if (swap_flg)
		swap (&tmp);
#endif
	return (gint32)tmp;
}

guint32 sranger_mk3_hwi_dev::uint_2_sranger_uint (guint32 x){
	guint16 tmp = x > 0xffff ? 0xffff : x < 0 ? 0 : x; // saturate
#if 0
	if (swap_flg)
		swap (&tmp);
#endif
	return (guint32)tmp;
}

gint32 sranger_mk3_hwi_dev::long_2_sranger_long (gint32 x){
#if 0
	if (swap_flg){
		swap (&x);
		return x;
	}

	if (target == SR_HWI_TARGET_C55){
		gint32 temp0, temp1, temp2, temp3;
		gchar  x32[4];
		
		
		temp0 = (x)       & 0xFF;
		temp1 = (x >> 8)  & 0xFF;
		temp2 = (x >> 16) & 0xFF;
		temp3 = (x >> 24) & 0xFF;

		x32[0] = temp2;
		x32[1] = temp3;
		x32[2] = temp0;
		x32[3] = temp1;

		x = *((gint32*)x32);
		
		return x;
	} else
		return x;
#else
	return x;
#endif
}




// Image Data FIFO read thread section
// ==================================================

// start_fifo_read:
// Data Transfer Setup:
// Prepare and Fork Image Data FIFO read thread

int sranger_mk3_hwi_dev::start_fifo_read (int y_start, 
				  int num_srcs0, int num_srcs1, int num_srcs2, int num_srcs3, 
				  Mem2d **Mob0, Mem2d **Mob1, Mem2d **Mob2, Mem2d **Mob3){
	PI_DEBUG_GP (DBG_L4, "IMAGE DATA FIFO READ STARTUP\n");
	if (num_srcs0 || num_srcs1 || num_srcs2 || num_srcs3){
		fifo_data_num_srcs[0] = num_srcs0;
		fifo_data_num_srcs[1] = num_srcs1;
		fifo_data_num_srcs[2] = num_srcs2;
		fifo_data_num_srcs[3] = num_srcs3;
		fifo_data_Mobp[0] = Mob0;
		fifo_data_Mobp[1] = Mob1;
		fifo_data_Mobp[2] = Mob2;
		fifo_data_Mobp[3] = Mob3;
		fifo_data_y_index = y_start; // if > 0, scan dir is "bottom-up"

		PI_DEBUG_GP (DBG_L4, "IMAGE DATA FIFO READ::THREAD CREATE\n");
                PI_DEBUG_PLAIN (DBG_L2,
                                "IMAGE DATA FIFO READ::ELSE... Source="  << DSPControlClass->Source <<
                                ", vis_Source=" << DSPControlClass->vis_Source
                                );

		fifo_read_thread = g_thread_new ("FifoReadThread3", FifoReadThread3, this);

		if (DSPControlClass->probe_trigger_raster_points > 0){
			PI_DEBUG_GP (DBG_L4, "IMAGE DATA FIFO READ::PROBE FIFO READ THREAD FORCE RESET+INIT\n");
			DSPControlClass->probe_trigger_single_shot = 0;
			ReadProbeFifo (thread_dsp, FR_FIFO_FORCE_RESET); // reset FIFO
			ReadProbeFifo (thread_dsp, FR_INIT); // init
		}
	}
	else{
		if (DSPControlClass->vis_Source & 0xffff){
			PI_DEBUG_GP (DBG_L4, "IMAGE DATA FIFO READ::CREATE PROBE FIFO READ THREAD\n");
			probe_fifo_read_thread = g_thread_new ("ProbeFifoReadThread3", ProbeFifoReadThread3, this);
		}
	}

	return 0;
}

// FifoReadThread3:
// Image Data FIFO read thread

gpointer FifoReadThread3 (void *ptr_sr){
	sranger_mk3_hwi_dev *sr = (sranger_mk3_hwi_dev*)ptr_sr;
	int ny = sr->fifo_data_Mobp[sr->fifo_data_num_srcs[0] ? 0:1][0]->GetNySub();

	SRANGER_DEBUG ("Starting Fifo Read, reading " << ny << " lines, " << "y_index: " << sr->fifo_data_y_index);

	sr->ReadLineFromFifo (SR_READFIFO_RESET); // init read fifo status

	if (sr->fifo_data_y_index == 0){ // top-down
		for (int yi=0; yi < ny; ++yi)
			if (sr->ReadLineFromFifo (yi))
				break;
	}else{ // bottom-up
		for (int yi=ny-1; yi >= 0; --yi)
			if (sr->ReadLineFromFifo (yi)) 
				break;
	}

	sr->ReadLineFromFifo (SR_EMPTY_PROBE_FIFO); // finish reading all FIFO's (probe may have to be emptied)

	SRANGER_DEBUG ("Fifo Read Done.");

        
	SRANGER_DEBUG ("BZ DATA STATISTICS: [8: " << sr->bz_statistics[0] 
                       << ", 16:" << sr->bz_statistics[1] 
                       << ", 24:" << sr->bz_statistics[2] 
                       << ", 32:" << sr->bz_statistics[3] 
                       << ", ST:" << sr->bz_statistics[4] 
                       << "]"
                       );
	

	return NULL;
}

// ReadLineFromFifo:
// read scan line from FIFO -- wait for data, and empty FIFO as quick as possible, 
// sort data chunks away int scan-mem2d objects
int sranger_mk3_hwi_dev::ReadLineFromFifo (int y_index){
	static int len[4] = { 0,0,0,0 };
	static int total_len = 0;
	static int fifo_reads = 0;
	static DATA_FIFO_MK3 dsp_fifo;
	static double max_fill = 0.;
	static int fifo_fill_delay = 0;
	int xi;
	static int readfifo_status = 0;
	static int rpos_tmp=0;
	static int wpos_tmp=0;
	static int fifo_fill=0;
	static GTimer *gt=NULL;
	static useconds_t fifo_read_sleep_us=500;

//		SRANGER_DEBUG ("ReadData: yindex=" << y_index << "---------------------------");

//	std::cout << "ReadLineFromFifo:" << y_index << std::endl;
		
// initiate and unlock scan process now!
	if (y_index == SR_READFIFO_RESET){
		for (int i=0; i<BZ_MAX_CHANNELS; ++i) bz_last[i] = 0L; 
		max_fill = 0.;
		bz_byte_pos = 0;
		bz_scale = 1.;
		bz_dsp_srcs = 0;
		bz_dsp_ns = 0;
		bz_push_mode = 0;
		rpos_tmp=0;
		wpos_tmp=0;
		fifo_fill=0;
		for (int i=0; i<5; ++i) bz_statistics[i]=0;
		readfifo_status = 0;
		fifo_block = 0;
		fifo_read_sleep_us=500;
		if (gt)
			g_timer_reset (gt);
		else
			gt = g_timer_new ();
		g_timer_start (gt);
		return 0;
	}

	// finish reading all FIFO's (probe may have to be emptied)
	if (y_index == SR_EMPTY_PROBE_FIFO){
		// check probe fifo
		if (DSPControlClass->probe_trigger_raster_points > 0 && DSPControlClass->Source){
			for (int i=0; i<10; ++i){ // about 1/4s finish time
				// free some cpu time now
				usleep(25000);
				ProbeFifoReadFunction3 (this, thread_dsp);
			}
		}
		return 0;
	}

	if (!readfifo_status){
		fifo_reads = 0; max_fill = 0;
		total_len = 0;
		for (int dir=0; dir < 4; ++dir){ // number of data chunks per scanline and direction
			if (!fifo_data_num_srcs[dir]) 
				len[dir] = 0;
			else
				len[dir] = fifo_data_Mobp[dir][0]->GetNxSub();
			total_len += len[dir];
//			std::cout << "Dir:" << dir << " L: " << len[dir] << std::endl;
		}

		// lseek (thread_dsp, magic_data.datafifo, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
		lseek (thread_dsp, magic_data.datafifo, SRANGER_MK23_SEEK_DATA_SPACE);
		sr_read (thread_dsp, &dsp_fifo, (MAX_WRITE_DATA_FIFO_SETUP)<<1, "RdLnFifo01");
		dsp_fifo.stall = 0; // unlock scanning
		check_and_swap (dsp_fifo.stall);
		sr_write (thread_dsp, &dsp_fifo, (MAX_WRITE_DATA_FIFO_SETUP)<<1, "RdLnFifo10");
		// to PC format
		check_and_swap (dsp_fifo.r_position);
		check_and_swap (dsp_fifo.w_position);
		readfifo_status = 1;
		PI_DEBUG_GP (DBG_L4, "FIFO STATUS: r=%04x w=%04x\n",dsp_fifo.r_position, dsp_fifo.w_position);
	}

	{
		int maxnum_f = MAX (MAX (MAX (fifo_data_num_srcs[0]&0x0f, fifo_data_num_srcs[1]&0x0f), fifo_data_num_srcs[2]&0x0f), fifo_data_num_srcs[3]&0x0f);
		int maxnum_l = MAX (MAX (MAX ((fifo_data_num_srcs[0]&0xf0)>>4, (fifo_data_num_srcs[1]&0xf0)>>4), (fifo_data_num_srcs[2]&0xf0)>>4), (fifo_data_num_srcs[3]&0xf0)>>4);
		float *linebuffer_f = new float[len[0]*(maxnum_f+maxnum_l)+1];
		for (int dir = 0; dir < 4 && ScanningFlg; ++dir){
			if (!fifo_data_num_srcs[dir]) continue;
			xi = 0;
			int count=0;
			do{
				// transfer data from DSP fifo into local fifo buffer -- empty DSP fifo
				if (count < 1){
					do {
						rpos_tmp &= DATAFIFO_MASK;

						// check probe fifo
						if (DSPControlClass->probe_trigger_raster_points > 0 && DSPControlClass->Source)
							ProbeFifoReadFunction3 (this, thread_dsp);
		
						// Get all and empty fifo
//						SRANGER_DEBUG_GP(".");
//						Seek for ATOMIC read
//						lseek (thread_dsp, magic_data.datafifo, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);

//						Regular seek for NON-ATOMIC read, testing!!!
						lseek (thread_dsp, magic_data.datafifo, SRANGER_MK23_SEEK_DATA_SPACE);
						sr_read  (thread_dsp, &dsp_fifo, sizeof (dsp_fifo));
						bz_need_reswap = 1;
						++fifo_reads;
						
						// to PC format
						check_and_swap (dsp_fifo.w_position);
						check_and_swap (dsp_fifo.length); // to PC
						wpos_tmp = dsp_fifo.w_position;
						
						if (dsp_fifo.w_position > 0x1000 || dsp_fifo.length > 0x1000)
							PI_DEBUG_GP (DBG_L4, "FIFO STATUS READ ERROR. DATA_FIFO_MK3=\n"
                                                                     "rpos=%08x wpos=%08x fill=%08x stall=%08x length=%08x\n", 
                                                                     dsp_fifo.r_position, dsp_fifo.w_position, dsp_fifo.fill, dsp_fifo.stall, dsp_fifo.length);

						while (wpos_tmp - rpos_tmp < 0) wpos_tmp += dsp_fifo.length;
						fifo_fill = wpos_tmp - rpos_tmp;

						if (fifo_fill > (dsp_fifo.length>>3)){
							if (fifo_read_sleep_us > 150)
								fifo_read_sleep_us -= 50;
						} else {
							if (fifo_read_sleep_us < 20000)
								fifo_read_sleep_us += 50;
						}

						usleep (fifo_read_sleep_us); // 5000
					} while (fifo_fill < 10 && ScanningFlg);

					// calc and update Fifo stats
					{
						double fifo_fill_percent;
//#define FIFO_DBG
#ifdef FIFO_DBG
						if (wpos_tmp > dsp_fifo.length){
							PI_DEBUG_GP (DBG_L4, "FIFO LOOPING: w=%04x (%d) r=%04x (%d) fill=%04x (%d)  L=%x %d\n",
								 dsp_fifo.w_position, dsp_fifo.w_position, 
								 dsp_fifo.r_position, dsp_fifo.r_position,
								 fifo_fill, fifo_fill, dsp_fifo.length, dsp_fifo.length);
						}
#endif
						// calc fill percentage
						fifo_fill = (int)(fifo_fill_percent=100.*(double)fifo_fill/(double)dsp_fifo.length);
						if (fifo_fill_percent > max_fill){
							max_fill = fifo_fill_percent;
						}
						g_free (AddStatusString);
						double stat_bzlen = (8.*bz_statistics[0] + 16.*bz_statistics[1] + 24.*bz_statistics[2] + 40.*bz_statistics[3] + 40.*bz_statistics[4])
							/(bz_statistics[0] + bz_statistics[1] + bz_statistics[2] + bz_statistics[3] + bz_statistics[4]);
						gulong dummy_ms;
						AddStatusString = g_strdup_printf ("[%d] Fifo: %.1f%% [%.1f%%] %.1fbit/pix(%d,%d,%d,%d,%d) {%d, %.3fMb/s %.3fms sleep}", 
										   y_index, fifo_fill_percent, max_fill, stat_bzlen, 
										   bz_statistics[0],bz_statistics[1],bz_statistics[2],
										   bz_statistics[3],bz_statistics[4],
										   fifo_reads, (8./1024./1024.)*fifo_reads * (double)sizeof(dsp_fifo) / g_timer_elapsed (gt, &dummy_ms),
										   1e-3*(double)fifo_read_sleep_us
							);
#ifdef FIFO_DBG
//						PI_DEBUG_GP (DBG_L4, AddStatusString);
						PI_DEBUG_GP (DBG_L4, "fifo buffer read #=%04d, w-r=%04x, dir=%d Stats: %s\n", fifo_reads, dsp_fifo.w_position - dsp_fifo.r_position, dir, AddStatusString);
#endif
					}
				}
						

				// transfer and convert data from fifo buffer
				rpos_tmp += count = FifoRead (rpos_tmp, wpos_tmp,
							      xi, fifo_data_num_srcs[dir], len[dir], 
							      linebuffer_f, dsp_fifo.buffer_ul);
						
			} while (xi < len[dir] && ScanningFlg); // until line completed, then dump data to GXSM

			// skip if scan was canceled
                        if (xi >= len[dir] && ScanningFlg){
                                // read data into linebuffer
                                int num_f = fifo_data_num_srcs[dir]&0x0f;
                                int num_l = (fifo_data_num_srcs[dir]&0xf0)>>4;
                                for (int i=0; i<(num_f+num_l); ++i){
                                        if (fifo_data_Mobp[dir][i])
                                                fifo_data_Mobp[dir][i]->PutDataLine (y_index, linebuffer_f+i*len[dir]);
                                }
                        }
 		}

		delete[] linebuffer_f;
	}

	fifo_data_y_index = y_index;

	// check probe fifo again
	if (DSPControlClass->probe_trigger_raster_points > 0 && DSPControlClass->Source)
		ProbeFifoReadFunction3 (this, thread_dsp);


	return (ScanningFlg) ? 0:1;
}

void Xdumpbuffer3(unsigned char *buffer, int size, int i0){
	int i,j,in;
	in=size;
	for(i=i0; i<in; i+=16){
		SRANGER_DEBUG_GP("W %06X:",(i>>1)&DATAFIFO_MASK);
		for(j=0; (j<16) && (i+j)<size; j++){
			SRANGER_DEBUG_GP(" %02x",buffer[(((i>>1)&DATAFIFO_MASK)<<1) + j]);
                }
		printf("   ");
		for(j=0; (j<16) && (i+j)<size; j++){
			if(isprint(buffer[j+i])){
				SRANGER_DEBUG_GP("%c",buffer[(((i>>1)&DATAFIFO_MASK)<<1) + j]);
			} else {
				SRANGER_DEBUG_GP(".");
                        }
                }
		SRANGER_DEBUG_GP("\n");
	}
}

int xindexx3=0;
int yindexy3=0;

//#define BZ_TEST
#ifdef BZ_TEST
// ------- simulate test ** SIMULATED/EXACT DSP LEVEL CODE HERE FOR TESTING ALGORITHM ** -------------

//#define BZ_MAX_CHANNELS 16
int bz_errors=0;
int testindex=0;
int testindex_max=0;
gint32 bz_testdata[2000];
gint32 bz_last[BZ_MAX_CHANNELS];
int bz_byte_pos=0;
DATA_FIFO        datafifo = {
		0, 0, // datafifo.r_position, datafifo.w_position
		0, 0, // datafifo.fill, datafifo.stall
		DATAFIFO_LENGTH, // datafifo.length
		1,    // datafifo.mode
		1,2,3,4,5,6,7,8 // first data...
};

#define BZ_PUSH_NORMAL   0x00000000UL // normal atomatic mode using size indicator bits 31,30:
// -- Info: THIS CONST NAMES .._32,08,16,24 ARE NOT USED, JUST FOR DOCUMENMTATION PURPOSE
#define BZ_PUSH_MODE_32  0x00000000UL // 40:32 => 0b00MMMMMM(8bit) 0xDDDDDDDD(32bit), M:mode bits, D:Data
#define BZ_PUSH_MODE_08  0xC0000000UL //  8:6  => 0b11DDDDDD
#define BZ_PUSH_MODE_16  0x80000000UL // 16:14 => 0b10DDDDDD DDDDDDDD
#define BZ_PUSH_MODE_24  0x40000000UL // 24:22 => 0b01DDDDDD DDDDDDDD DDDDDDDD
#define BZ_PUSH_MODE_32_START 0x01000000UL // if 31,30=0 bits 29..0 available for special modes: 0x02..0x3f
// more modes free for later options and expansions
#define BZ_PUSH_MODE_32_z     0x3f000000UL
DSP_ULONG bz_push_mode=0;

void bz_init(){ 
	int i; 
	for (i=0; i<BZ_MAX_CHANNELS; ++i) bz_last[i] = 0L; 
	bz_byte_pos=0; datafifo.buffer.ul[0] = 0UL;

	datafifo.w_position = 0;
}

void bz_push(int i, long x){
	union { DSP_INT32 l; DSP_UINT32 ul; } delta;
	int bits;

	bz_testdata[testindex++] = x;

// ERRORS so far to DSP code: norm == 0 test!! missed delta.ul ">>2" in add on parts below!! Fix "else" construct of 32bit full!!

	delta.l = x - bz_last[i];
	bz_last[i] = x;

	if (bz_push_mode) 
		goto bz_push_mode_32;

	bits  = _norm (delta.l); // THIS NORM RETURNs 32 for arg=0
        SRANGER_DEBUG_SIG (
                       "BZ_PUSH(i=" << i << ", ***x=" << std::hex << x << std::dec
                       << ") D.l=" << delta.l << " bits=" << bits);
	if (bits > 26){ // 8 bit package, 6 bit data
		delta.l <<= 26;
                SRANGER_DEBUG_SIG (
                               ">26** 8:6bit** BZdelta.l=" << std::hex << delta.l << std::dec
                               << " bz_byte_pos=" << bz_byte_pos
                               );
		switch (bz_byte_pos){
		case 0:  datafifo.buffer.ul[datafifo.w_position>>1]  =  0xC0000000UL | (delta.ul>>2)       ; bz_byte_pos=1; break;
		case 1:  datafifo.buffer.ul[datafifo.w_position>>1] |= (0xC0000000UL | (delta.ul>>2)) >>  8; bz_byte_pos=2; break;
		case 2:  datafifo.buffer.ul[datafifo.w_position>>1] |= (0xC0000000UL | (delta.ul>>2)) >> 16; bz_byte_pos=3; break;
		default: datafifo.buffer.ul[datafifo.w_position>>1] |= (0xC0000000UL | (delta.ul>>2)) >> 24; bz_byte_pos=0;
			datafifo.w_position += 2;
			datafifo.w_position &= DATAFIFO_MASK;
			break;
		}
	} else if (bits > 18){ // 16 bit package, 14 bit data
		delta.l <<= 18;
                SRANGER_DEBUG_SIG (
                               << ">18**16:14bit** BZdelta.l=" << std::hex << delta.l << std::dec
                               << " bz_byte_pos=" << bz_byte_pos
                               );
		switch (bz_byte_pos){
		case 0:  datafifo.buffer.ul[datafifo.w_position>>1]  =  0x80000000UL | (delta.ul>>2)       ; bz_byte_pos=2; break;
		case 1:  datafifo.buffer.ul[datafifo.w_position>>1] |= (0x80000000UL | (delta.ul>>2)) >>  8; bz_byte_pos=3; break;
		case 2:  datafifo.buffer.ul[datafifo.w_position>>1] |= (0x80000000UL | (delta.ul>>2)) >> 16; bz_byte_pos=0;
			datafifo.w_position += 2;
			datafifo.w_position &= DATAFIFO_MASK;
			break;
		default: datafifo.buffer.ul[datafifo.w_position>>1] |= (0x80000000UL | (delta.ul>>2)) >> 24; 
			datafifo.w_position += 2;
			datafifo.w_position &= DATAFIFO_MASK;
			datafifo.buffer.ul[datafifo.w_position>>1] = ((delta.ul>>2)&0x00ff0000UL) << 8; bz_byte_pos=1; break;
		}
	} else if (bits > 10){ // 24 bit package, 22 bit data
		delta.l <<= 10;
                SRANGER_DEBUG_SIG (
                               ">10**24:22bit** BZdelta.l=" << std::hex << delta.l << std::dec
                               << " bz_byte_pos=" << bz_byte_pos
                               );
		switch (bz_byte_pos){
		case 0:  datafifo.buffer.ul[datafifo.w_position>>1]  =  0x40000000UL | (delta.ul>>2)       ; bz_byte_pos=3; break;
		case 1:  datafifo.buffer.ul[datafifo.w_position>>1] |= (0x40000000UL | (delta.ul>>2)) >>  8; bz_byte_pos=0;
			datafifo.w_position += 2;
			datafifo.w_position &= DATAFIFO_MASK;
			break;
		case 2:  datafifo.buffer.ul[datafifo.w_position>>1] |= (0x40000000UL | (delta.ul>>2)) >> 16;
			datafifo.w_position += 2;
			datafifo.w_position &= DATAFIFO_MASK;
			datafifo.buffer.ul[datafifo.w_position>>1] = ((delta.ul>>2)&0x0000ff00UL) << 16; bz_byte_pos=1; break;
		default: datafifo.buffer.ul[datafifo.w_position>>1] |= (0x40000000UL | (delta.ul>>2)) >> 24;
			datafifo.w_position += 2;
			datafifo.w_position &= DATAFIFO_MASK;
			datafifo.buffer.ul[datafifo.w_position>>1] = ((delta.ul>>2)&0x00ffff00UL) << 8; bz_byte_pos=2; break;
		}
	} else { // full 32 bit data -- special on byte alignment -- 6 spare unused bits left!
	bz_push_mode_32:
		delta.l = x;
                SRANGER_DEBUG_SIG (
                               "ELSE**40:32bit** BZdelta.l=" << std::hex << delta.l << std::dec
                               << " bz_byte_pos=" << bz_byte_pos
                               );
		bits=datafifo.w_position>>1;

		switch (bz_byte_pos){
		case 0:  datafifo.buffer.ul[datafifo.w_position>>1]  =  bz_push_mode | (delta.ul >> 8); // 0xMMffffff..
			datafifo.w_position += 2;
			datafifo.w_position &= DATAFIFO_MASK;
			datafifo.buffer.ul[datafifo.w_position>>1] = delta.ul << 24; bz_byte_pos=1; break;
		case 1:  datafifo.buffer.ul[datafifo.w_position>>1] |= (bz_push_mode | (delta.ul >> 8)) >> 8; // 0x==MMffff....
			datafifo.w_position += 2;
			datafifo.w_position &= DATAFIFO_MASK;
			datafifo.buffer.ul[datafifo.w_position>>1] = delta.ul << 16; bz_byte_pos=2; break;
		case 2:  datafifo.buffer.ul[datafifo.w_position>>1] |= (bz_push_mode | (delta.ul >> 8)) >> 16; // 0x====MMff......
			datafifo.w_position += 2;
			datafifo.w_position &= DATAFIFO_MASK;
			datafifo.buffer.ul[datafifo.w_position>>1] = delta.ul <<  8; bz_byte_pos=3; break;
		default: datafifo.buffer.ul[datafifo.w_position>>1] |= bz_push_mode >> 24; // 0x====MM........
			datafifo.w_position += 2;
			datafifo.w_position &= DATAFIFO_MASK;
			datafifo.buffer.ul[datafifo.w_position>>1] = delta.ul;
			datafifo.w_position += 2;
			datafifo.w_position &= DATAFIFO_MASK; bz_byte_pos=0;
			break;
		}

		Xdumpbuffer3 ((unsigned char*)&datafifo.buffer.ul[bits-4], 32, 0);
	}
//	std::cout << "----> Fin bz_byte_pos=" << bz_byte_pos << std::endl;;
}

void run_test(){
	memset ((unsigned char*)datafifo.buffer.ul, 0, DATAFIFO_LENGTH<<1);
	bz_init ();
	Xdumpbuffer3 ((unsigned char*)datafifo.buffer.ul, 32, 0);
	gint32 tmp = (0x00000011&0x0000ffffUL << 16) | (DSP_ULONG)2;

	bz_push_mode = BZ_PUSH_MODE_32_START; 
	bz_push (0, tmp);
	bz_push_mode = BZ_PUSH_NORMAL;

	bz_push (1,0x100);
	bz_push (2,0x200);
	for (int i=0; i<=4; ++i){
		bz_push (1,0x100+i);
		bz_push (2,0x200+2*i);
	}
	for (int i=4; i>=0; --i){
		bz_push (1,0x100+i);
		bz_push (2,0x200+2*i);
	}

	bz_push (1,0x11223344);
	bz_push (2,0xaabbccdd);
	bz_push (1,0x55667788);
	bz_push (2,0xeeff9900);

	bz_push (1,0x11223344);
	bz_push (2,0xaabbccdd);
	bz_push (1,0x55667788);
	bz_push (2,0xeeff9900);

	for (int i=0; i<=4; ++i){
		bz_push (1,0x100+0x80*i);
		bz_push (2,0x200+2*0x80*i);
	}

	for (int i=4; i>=0; --i){
		bz_push (1,0x100+0x80*i);
		bz_push (2,0x200+2*0x80*i);
	}
	for (int i=0; i<=4; ++i){
		bz_push (1,0x100+0x800*i);
		bz_push (2,0x200+2*0x800*i);
	}
	for (int i=4; i>=0; --i){
		bz_push (1,0x100+0x800*i);
		bz_push (2,0x200+2*0x800*i);
	}
	for (int i=0; i<=4; ++i){
		bz_push (1,0x100+0x8000*i);
		bz_push (2,0x200+2*0x8000*i);
	}
	for (int i=4; i>=0; --i){
		bz_push (1,0x100+0x8000*i);
		bz_push (2,0x200+2*0x8000*i);
	}

	bz_push (1,0x11223344);
	bz_push (2,0xaabbccdd);
	bz_push (1,0x55667788);
	bz_push (2,0xeeff9900);

	srand((unsigned)time(0)); 
	int r; 
	int lowest=-100, highest=100; 
	double range=(highest-lowest)+1.; 
	for (int k=0; k<5; ++k){
		for (int i=0; i<=50; ++i){
			r = lowest+int(range*random()/(RAND_MAX + 1.0)); 
			bz_push (1,0x100*k+0x800*i+r);
			r = lowest+int(range*random()/(RAND_MAX + 1.0)); 
			bz_push (2,0x200*k+2*0x800*i+r);
		}
		for (int i=50; i>=0; --i){
			r = lowest+int(range*random()/(RAND_MAX + 1.0)); 
			bz_push (1,0x100*k+0x800*i+r);
			r = lowest+int(range*random()/(RAND_MAX + 1.0)); 
			bz_push (2,0x200*k+2*0x800*i+r);
		}
	}
	lowest=-5000, highest=5000; 
	range=(highest-lowest)+1.; 
	for (int k=0; k<3; ++k){
		for (int i=0; i<=50; ++i){
			r = lowest+int(range*random()/(RAND_MAX + 1.0)); 
			bz_push (1,0x100*k+0x800*i+r);
			r = lowest+int(range*random()/(RAND_MAX + 1.0)); 
			bz_push (2,0x200*k+2*0x800*i+r);
		}
		for (int i=50; i>=0; --i){
			r = lowest+int(range*random()/(RAND_MAX + 1.0)); 
			bz_push (1,0x100*k+0x800*i+r);
			r = lowest+int(range*random()/(RAND_MAX + 1.0)); 
			bz_push (2,0x200*k+2*0x800*i+r);
		}
	}
	double dlowest=-2147483645., dhighest=2147483645.; 
//	lowest=-2147483645, highest=2147483645; 
	range=(dhighest-dlowest)+1.; 
	for (int k=0; k<100; ++k){
		r = (gint32)(dlowest+(range*random()/(RAND_MAX + 1.0)));
		bz_push (1,r);
		bz_push (2,r>>1);
	}
}
// -------------- END DSP LEVEL CODE TEST
#endif



// FifoRead:
// Read data from FIFO ring buffer (ring) now in host memory and convert and sort into temporary secondary buffer (linear)
// Short-Type Data
int sranger_mk3_hwi_dev::FifoRead (int start, int end, int &xi, int num_srcs, int len, float *buffer_f, DSP_UINT32 *fifo_l){
	int count=0;
	int num_f = num_srcs&0x0f;
	int num_l = (num_srcs&0xf0)>>4;

	int fi_delta = end-start;
 // -- DSP pads at end of scan with full 32-bit 3F00000000 x MAX_CHANNELS -- so packed record is never longer than this:
	const int max_length = ((5*(num_srcs+2))>>1);

//	PI_DEBUG_GP (DBG_L4, "FIFO read attempt: FIFO BUFFER [start=%x :: end=%x]):\n", start, end);
//	Xdumpbuffer3 ((unsigned char*)fifo_l, (end+16)<<1, start<<1);

	if (end > 0x4000 || start > 0x4000 || len > 0x4000){
		PI_DEBUG_GP (DBG_L4, "FIFO read ERROR ranges out of range:\n( xi=%d numsrcs=%d, len=%d [start=%x :: end=%x]):\n", xi, num_srcs, len, start, end);
		return -1;
	}

	while (end < start) end += DATAFIFO_LENGTH;

	end -= max_length;

// make sure we have at least max_length data ready before proceeding
// -- need to have at least one full record, of unkown length, but smaller than the end pad.
	if (start > end)
		return 0;

	if (fifo_block == 0) yindexy3=0;
	++fifo_block;

//	PI_DEBUG_GP (DBG_L4, "FIFO read[%6d]( xi=%d numsrcs=%d, len=%d FIFO BUFFER CIRC [start=%x.%d :: end=%x]):\n", fifo_block, xi, num_srcs, len, start, bz_byte_pos, end);
	


#ifdef BZ_TEST
	int swfold;
//	Xdumpbuffer3 ((unsigned char*)fifo_l, (end-start)<<1, start<<1);

	std::cout << "BZ_TEST_START **************" << std::endl;
	std::cout << "BZ_TEST_START -- simulating TWO channels, must select two soure channels in GXSM befor hit scan!!" << std::endl;
	std::cout << "BZ_TEST_START -- TEST SET MUST FIT FULLY IN DATAFIFOLENGTH=" << DATAFIFO_LENGTH << std::endl;
	run_test ();
	std::cout << "BZ_TEST_END   **************" << std::endl;
//		std::cout << "-------------- ABORTING HERE WITH EXIT(0) TEST VERSION SORRY." << std::endl;
//		exit(0);
	std::cout << "-------------- DATA OVERRIDE ------------ TEST VERSION SORRY." << std::endl;
	memcpy ((unsigned char*)fifo_l, (unsigned char*)datafifo.buffer.ul, DATAFIFO_LENGTH<<1);
	start=0; end=DATAFIFO_LENGTH-1; swfold=swap_flg; swap_flg = 0; target = SR_HWI_TARGET_C54;
	Xdumpbuffer3 ((unsigned char*)fifo_l, end, 0);

	testindex_max = testindex-1;
	std::cout << "-------------- TEST DATA COUNT: " << testindex_max << std::endl;
	testindex=0;


	std::cout << "-------------- SWAP VERIFY ------------" << std::endl;
	gint32  tt = 0x11223344;
	std::cout << "TESTi=" << std::hex << tt << std::dec << std::endl;
	check_and_swap (tt);
	std::cout << "TESTf=" << std::hex << tt << std::dec << std::endl;
#endif

	for (int fi=start; count < end-start; ){
		int j=1;
		LNG x;

#if 0
		if (fi == 0){
			PI_DEBUG_GP (DBG_L4, "FIFO top of buffer at fi=0x%04x / count=%d\n-------- Fifo Debug:\n", fi, count);
			PI_DEBUG_GP (DBG_L4, "FIFO read[%6d]( xi=%d numsrcs=%d, len=%d FIFO BUFFER CIRC [start=%x.%d :: end=%x]):\n", fifo_block, xi, num_srcs, len, start, bz_byte_pos, end);
			Xdumpbuffer3 ((unsigned char*)fifo_l, (end+16)<<1, start<<1);
		}
#endif

		xindexx3 = xi; // dbg only

		for (int i=0; i<(num_f+num_l); ++i){
			if (fi >= (end+max_length)){
				PI_DEBUG_GP (DBG_L4, "ERROR buffer valid range overrun: i of num_f=%d fi=%4x, x=%d, y=%d\n-------- Fifo Debug:\n", i, fi, xindexx3, yindexy3);
				PI_DEBUG_GP (DBG_L4, "FIFO read[%6d]( xi=%d numsrcs=%d, len=%d FIFO BUFFER CIRC [start=%x.%d :: end=%x]):\n", fifo_block, xi, num_srcs, len, start, bz_byte_pos, end);
				Xdumpbuffer3 ((unsigned char*)fifo_l, (end+16)<<1, start<<1);
				return count;
			}
			x = bz_unpack (j, fifo_l, fi, count);
			if (bz_push_mode <= 0x3f)
				switch (bz_push_mode){
				case 0: break; // 40:32 bit full data coming in - OK, proceed.
				case 1: // decode START package and start over
					bz_dsp_ns   =  x & 0xffff;
					bz_dsp_srcs = (x >> 16) & 0xffff;
					bz_scale = 1./(double)bz_dsp_ns;
					--i;
//				PI_DEBUG_GP (DBG_L4, "FIFO read: START_PACKAGE! ns=%d, srcs=%4x\n", bz_dsp_ns, bz_dsp_srcs);
					continue;
				case 0x3f:
					PI_DEBUG_GP (DBG_L4, "FIFO read: AS 0x3F END MARK DETECTED (should not get there, ERROR?)\n-------- Fifo Debug:\n");
					PI_DEBUG_GP (DBG_L4, "FIFO read[%6d]( xi=%d numsrcs=%d, len=%d FIFO BUFFER CIRC [start=%x.%d :: end=%x]):\n", fifo_block, xi, num_srcs, len, start, bz_byte_pos, end);
					Xdumpbuffer3 ((unsigned char*)fifo_l, (end+16)<<1, start<<1);
					return count;
					break;
				default:
					PI_DEBUG_GP (DBG_L4, "FIFO read: 32 Bit Unkown Package MARK %2x DETECTED at fi=0x%04x / count=%d (ERROR?)\n-------- Fifo Debug:\n", bz_push_mode, fi, count);
					PI_DEBUG_GP (DBG_L4, "FIFO read[%6d]( xi=%d numsrcs=%d, len=%d FIFO BUFFER CIRC [start=%x.%d :: end=%x]):\n", fifo_block, xi, num_srcs, len, start, bz_byte_pos, end);
					Xdumpbuffer3 ((unsigned char*)fifo_l, (end+16)<<1, start<<1);
					return count;
					break;
				}
			int num_mix = bz_dsp_srcs & 1; 
			for (int jj=1; jj<=4; ++jj) if (bz_dsp_srcs & (1<<jj)) ++num_mix;  // count MIX* srcs -- unscaled 32bit

			if (i==0 && bz_dsp_srcs&1) // put 16.16 Topo data chunks from packed stream
				buffer_f[xi+i*len] = (float)((double)x/65536.);
			else if (i < num_mix && bz_dsp_srcs&(2+4+8+0x10)) // put MIX* data chunks from packed stream
			        buffer_f[xi+i*len] = (float)((double)x/256.);
			else if (i<num_f) // put 32.0 summed data chunks from packed stream and normalize, store as float
				buffer_f[xi+i*len] = (float)(bz_scale*(double)x);
			else // put plain 32.0 data chunks
				buffer_f[xi+i*len] = x;

// DBG STUFF
//			if (i==0) buffer_f[xi+i*len] = (float)fi+(float)bz_byte_pos/4.; // dbg ovr.
//			if (i==1) buffer_f[xi+i*len] = (float)fi_delta; // dbg ovr.

			++j;
		}

#ifdef BZ_TEST
		if (testindex >= testindex_max){ // || ++xi == len){
			std::cout << "------------------------------------------------" << std::endl;
			std::cout << "FIFO read BZ DATA STATISTICS: [8: " << bz_statistics[0] 
				  << ", 16:" << bz_statistics[1] 
				  << ", 24:" << bz_statistics[2] 
				  << ", 32:" << bz_statistics[3] 
				  << ", ST:" << bz_statistics[4] 
				  << "]" << std::endl;


			std::cout << "TEST DONE with " << testindex << " data values. Total Error Count: " << bz_errors << std::endl;
			break;
		}
#else
		if( ++xi == len){ // full line finished
			++yindexy3;
			break;
		}
#endif
	}	

//#define BZ_TEST_EXIT
#ifdef BZ_TEST_EXIT
	swap_flg = swfold;
	target = SR_HWI_TARGET_C55;

	std::cout << "-------------- ABORTING HERE WITH EXIT(0) TEST VERSION SORRY." << std::endl;
	exit(0);
#endif

#if 0
	PI_DEBUG_GP (DBG_L4, "FIFO read done[y=%d]. Count=%x.%d\n", yindexy3, count, bz_byte_pos);
	for (int j=0; j<num_f; ++j){
		PI_DEBUG_GP (DBG_L4, "Buffer[%d,xi#%d]={",j,xi);
		for (int i=0; i<xi; ++i)
			PI_DEBUG_GP (DBG_L4, "%g,",buffer_f[i+j*len]);
		PI_DEBUG_GP (DBG_L4, "}\n");
	}
#endif
	return count;
}

LNG sranger_mk3_hwi_dev::bz_unpack (int i, DSP_UINT32 *fifo_l, int &fi, int &count){
	int bits_mark[4] = { 32, 10, 18, 26 };
	union { DSP_INT32 l; DSP_UINT32 ul; } x;
	union { DSP_INT32 l; DSP_UINT32 ul; } delta;
	int bits;

	// manage swap, fi and count here!

	if (bz_byte_pos == 0 || bz_need_reswap){
		check_and_swap (fifo_l[fi>>1]);
		bz_need_reswap = 0;
	}

	x.l = fifo_l[fi>>1];

	switch (bz_byte_pos){
	case 0:  bits = bits_mark[(x.ul & 0xC0000000UL)>>30]; break;
	case 1:  bits = bits_mark[(x.ul & 0x00C00000UL)>>22]; break;
	case 2:  bits = bits_mark[(x.ul & 0x0000C000UL)>>14]; break;
	default: bits = bits_mark[(x.ul & 0x000000C0UL)>> 6]; break;
	}
	
//	PI_DEBUG_GP (DBG_L4, "BZ_UNPACK(%6x,%6d) [%4d, %4d, %d] bits=%2d @ pos=%d x.ul=%08x ",fi,count,xindexx3, yindexy3, i, bits, bz_byte_pos, x.ul);
	switch (bits){
	case 26: // 8 bit package, 6 bit data
		++bz_statistics[0];
		bz_push_mode = 0xC0;
		switch (bz_byte_pos){
		case 0:  delta.ul = (x.ul & 0x3f000000) <<  2; delta.l >>= 26; bz_byte_pos=1; break;
		case 1:  delta.ul = (x.ul & 0x003f0000) << 10; delta.l >>= 26; bz_byte_pos=2; break;
		case 2:  delta.ul = (x.ul & 0x00003f00) << 18; delta.l >>= 26; bz_byte_pos=3; break;
		default: delta.ul = (x.ul & 0x0000003f) << 26; delta.l >>= 26; bz_byte_pos=0; fi+=2; fi&=DATAFIFO_MASK; count+=2; break;
		}
		bz_last[i] += delta.l;
		break;
	case 18: // 16 bit package, 14 bit data
		++bz_statistics[1];
		bz_push_mode = 0x80;
		switch (bz_byte_pos){
		case 0:  delta.ul = (x.ul & 0x3fff0000) <<  2; delta.l >>= 18; bz_byte_pos=2; break;
		case 1:  delta.ul = (x.ul & 0x003fff00) << 10; delta.l >>= 18; bz_byte_pos=3; break;
		case 2:  delta.ul = (x.ul & 0x00003fff) << 18; delta.l >>= 18; bz_byte_pos=0; fi+=2; fi&=DATAFIFO_MASK; count+=2; break;
		default: delta.ul = (x.ul & 0x0000003f) << 26; fi+=2; fi&=DATAFIFO_MASK; count+=2;
			check_and_swap (fifo_l[fi>>1L]);
			x.l = fifo_l[fi>>1L];
//			PI_DEBUG_GP (DBG_L4, " ... 3: x.ul=%08x", x.ul);
			delta.ul |= (x.ul & 0xff000000) >>  6; delta.l >>= 18; bz_byte_pos=1; break;
		}
		bz_last[i] += delta.l;
		break;
	case 10: // 24 bit package, 22 bit data
		++bz_statistics[2];
		bz_push_mode = 0x40;
		switch (bz_byte_pos){
		case 0:  delta.ul = (x.ul & 0x3fffff00) <<  2; delta.l >>= 10; bz_byte_pos=3; break;
		case 1:  delta.ul = (x.ul & 0x003fffff) << 10; delta.l >>= 10; bz_byte_pos=0; fi+=2; fi&=DATAFIFO_MASK; count+=2; break;
		case 2:  delta.ul = (x.ul & 0x00003fff) << 18; fi+=2; fi&=DATAFIFO_MASK; count+=2;
			check_and_swap (fifo_l[fi>>1L]);
			x.l = fifo_l[fi>>1L];
//			PI_DEBUG_GP (DBG_L4, " ... 2: x.ul=%08x", x.ul);
			delta.ul |= (x.ul & 0xff000000) >> 14; delta.l >>= 10; bz_byte_pos=1; break;
		default: delta.ul = (x.ul & 0x0000003f) << 26; fi+=2; fi&=DATAFIFO_MASK; count+=2;
			check_and_swap (fifo_l[fi>>1L]);
			x.l = fifo_l[fi>>1L];
//			PI_DEBUG_GP (DBG_L4, " ... 3: x.ul=%08x", x.ul);
			delta.ul |= (x.ul & 0xffff0000) >>  6; delta.l >>= 10; bz_byte_pos=2; break;
		}
		bz_last[i] += delta.l;
		break;
	default: // 40bit package, full 32 bit data, 6 spare uinused bits, byte aligned. ...0x00ffffffff...
		switch (bz_byte_pos){
		case 0:  delta.ul = (x.ul & 0x00ffffff) <<  8;
			bz_push_mode = (x.ul & 0xff000000) >> 24; fi+=2; fi&=DATAFIFO_MASK; count+=2;
			check_and_swap (fifo_l[fi>>1L]);
			x.l = fifo_l[fi>>1L];
//			PI_DEBUG_GP (DBG_L4, " bzp=%02x ... 0: x.ul=%08x", bz_push_mode, x.ul);
			delta.ul |= (x.ul & 0xff000000) >> 24; bz_byte_pos=1; break;
		case 1:  delta.ul = (x.ul & 0x0000ffff) << 16;
			bz_push_mode = (x.ul & 0x00ff0000) >> 16; fi+=2; fi&=DATAFIFO_MASK; count+=2;
			check_and_swap (fifo_l[fi>>1L]);
			x.l = fifo_l[fi>>1L];
//			PI_DEBUG_GP (DBG_L4, " bzp=%02x ... 1: x.ul=%08x", bz_push_mode, x.ul);
			delta.ul |= (x.ul & 0xffff0000) >> 16; bz_byte_pos=2; break;
		case 2:  delta.ul = (x.ul & 0x000000ff) << 24;
			bz_push_mode = (x.ul & 0x0000ff00) >> 8; fi+=2; fi&=DATAFIFO_MASK; count+=2;
			check_and_swap (fifo_l[fi>>1L]);
			x.l = fifo_l[fi>>1L];
//			PI_DEBUG_GP (DBG_L4, " bzp=%02x ... 2: x.ul=%08x", bz_push_mode, x.ul);
			delta.ul |= (x.ul & 0xffffff00) >>  8; bz_byte_pos=3; break;
		default:  	 
			bz_push_mode = (x.ul & 0x000000ff); fi+=2; fi&=DATAFIFO_MASK; count+=2;
			check_and_swap (fifo_l[fi>>1L]);
			x.l = fifo_l[fi>>1L];
//			PI_DEBUG_GP (DBG_L4, " bzp=%02x ... 3: x.ul=%08x", bz_push_mode, x.ul);
			delta.ul = x.ul; fi+=2; fi&=DATAFIFO_MASK; count+=2; bz_byte_pos=0; break;
		}
		switch (bz_push_mode){
		case 0: 
			++bz_statistics[3];
			bz_last[i] = delta.l;
			break; // normal 32 operation
		case 1:
			++bz_statistics[4];
			bz_last[i=0] = delta.l;
			break; // special mode 1 "START", put to index 0!
		default: break;
		}
		break;
	}


#ifdef BZ_TEST
	gint32 v = bz_last[i] - bz_testdata[testindex];
	if (v != 0)
		bz_errors++;

//	std::cout << "  ==["<<i<<"]d=" <<delta.l<<" ==> " << std::hex << bz_last[i] << " V(" << bz_testdata[testindex] << ")" << (v==0?"OK":"ERROR") << std::dec << std::endl;
	testindex++;
#else

//	PI_DEBUG_GP (DBG_L4, "[bzp=%02x] delta.l=%08x (%6g) N[%d] ==> %08x = %6g\n", bz_push_mode, delta.l, (double)delta.l, _norm(delta.l), bz_last[i], (double)bz_last[i]);

#endif

	return bz_last[i];
}


// ==================================================
//
// Probe Data FIFO read thread section
//
// ==================================================
 
// FIFO watch verbosity...
//# define LOGMSGS0(X) PI_DEBUG (DBG_L1, X)
# define LOGMSGS0(X)

# define LOGMSGS(X) PI_DEBUG (DBG_L2, X)
//# define LOGMSGS(X)

//# define LOGMSGS1(X) PI_DEBUG (DBG_L4, X)
# define LOGMSGS1(X)

//# define LOGMSGS2(X) PI_DEBUG (DBG_L5, X)
# define LOGMSGS2(X)

// ProbeFifoReadThread:
// Independent ProbeFifoRead Thread (manual probe)

gpointer ProbeFifoReadThread3 (void *ptr_sr){
#define MODE_READ                 0 // FALSE
#define MODE_FINISH_NORMAL        1 // TRUE
#define MODE_FINISHED_OK         -1  // FINISH / ERRORs
#define MODE_FINISHED_FR_ERROR   -11 // ...
#define MODE_FINISHED_TIMEOUT    -12
#define MODE_FINISHED_FIFO_ERROR -13

	int finish_flag=MODE_READ;
	sranger_mk3_hwi_dev *sr = (sranger_mk3_hwi_dev*)ptr_sr;
	if (!DSPControlClass->Source){
		LOGMSGS ( "ProbeFifoReadThread3 exiting, no sources selected for transfer." << std::endl);
		return NULL;
	}
	if (sr->probe_fifo_thread_active){
		LOGMSGS ( "ProbeFifoReadThread3 ERROR: Attempt to start again while in progress! [#" << sr->probe_fifo_thread_active << "]" << std::endl);
		return NULL;
	}
	sr->probe_fifo_thread_active++;
	DSPControlClass->probe_ready = FALSE;

	if (DSPControlClass->probe_trigger_single_shot)
		finish_flag=MODE_FINISH_NORMAL; // TRUE

	clock_t timeout = clock() + (clock_t)(CLOCKS_PER_SEC*(0.5+sr->probe_time_estimate/22000.));

	LOGMSGS ( "ProbeFifoReadThread3 START, FR_INIT  " << (DSPControlClass->probe_trigger_single_shot ? "Single":"Multiple") << "-VP[#" 
		  << sr->probe_fifo_thread_active << "] Timeout is set to:" 
		  << (timeout-clock()) << "Clks (incl. 0.5s Reserve) (" << ((double)sr->probe_time_estimate/22000.) << "s)" << std::endl);

	sr->ReadProbeFifo (sr->probe_thread_dsp, FR_INIT); // init

	int i=1;
	while (sr->is_scanning () || finish_flag > 0){
                
                if (DSPControlClass->current_auto_flags & FLAG_AUTO_PLOT)
                        DSPControlClass->Probing_graph_update_thread_safe (0);
		++i;
		switch (sr->ReadProbeFifo (sr->probe_thread_dsp)){
		case RET_FR_NOWAIT:
			LOGMSGS2 ( ":NOWAIT:" << i << " TmoClk=" << (timeout-clock()) << std::endl);
			continue;
		case RET_FR_WAIT:
			LOGMSGS2 ( ":WAIT:" << i << " TmoClk=" << (timeout-clock()) << std::endl);
//			usleep(50000);
			usleep(1500);
			if (finish_flag && clock() > timeout)
				finish_flag=MODE_FINISHED_TIMEOUT;
			continue;
		case RET_FR_OK:
			LOGMSGS2 ( ":OK:" << i << " TmoClk=" << (timeout-clock()) << std::endl);
			continue;
		case RET_FR_ERROR:
			LOGMSGS ( ":ERROR:" << i << " TmoClk=" << (timeout-clock()) << std::endl);
			finish_flag=MODE_FINISHED_FR_ERROR;
			continue;
		case RET_FR_FCT_END: 
			LOGMSGS ( ":FCT_END:" << i << " TmoClk=" << (timeout-clock()) << std::endl);
			if (finish_flag > 0){

				if (DSPControlClass->current_auto_flags & FLAG_AUTO_PLOT)
                                        DSPControlClass->Probing_graph_update_thread_safe (1);

				if (DSPControlClass->current_auto_flags & FLAG_AUTO_SAVE)
					DSPControlClass->Probing_save_callback (NULL, DSPControlClass);
                                        
				finish_flag=MODE_FINISHED_OK;
				continue;
			}
			DSPControlClass->push_probedata_arrays ();
			DSPControlClass->init_probedata_arrays ();

			// reset timeout
			timeout = clock() + (clock_t)(CLOCKS_PER_SEC*(0.5+sr->probe_time_estimate/22000.));
			continue;
		}
		LOGMSGS ( ":FIFO THREAD ERROR DETECTION:" << i << " TmoClk=" << (timeout-clock()) << std::endl);
		finish_flag=MODE_FINISHED_FIFO_ERROR;
	}

	switch (finish_flag){
	case MODE_FINISHED_OK:
		LOGMSGS ( "ProbeFifoReadThread3 FINISH OK. VP[#" << sr->probe_fifo_thread_active << "] Timeout left (positive is OK):" << (timeout-clock()) << std::endl); break;
	case MODE_FINISHED_FIFO_ERROR:
		LOGMSGS ( "ProbeFifoReadThread3 FINISHED with FIFO ERROR. Single-VP[#" << sr->probe_fifo_thread_active << "] Timeout left (positive is OK):" << (timeout-clock()) << std::endl); break;
	case MODE_FINISHED_TIMEOUT:
		LOGMSGS ( "ProbeFifoReadThread3 FINISHED with FIFO TIMEOUT. Single-VP[#" << sr->probe_fifo_thread_active << "] Timeout left (positive is OK):" << (timeout-clock()) << std::endl); break;
	case MODE_FINISHED_FR_ERROR:
		LOGMSGS ( "ProbeFifoReadThread3 FINISHED with FR_ERROR.  Single-VP[#" << sr->probe_fifo_thread_active << "] Timeout left (positive is OK):" << (timeout-clock()) << std::endl); break;
	default:
		LOGMSGS ( "ProbeFifoReadThread3 FINISHED with EXCEPTION ERROR.  Single-VP[#" << sr->probe_fifo_thread_active << "] Timeout left (positive is OK):" << (timeout-clock()) << std::endl); break;
	}

	sr->ReadProbeFifo (sr->probe_thread_dsp, FR_FINISH); // finish

	--sr->probe_fifo_thread_active;
	DSPControlClass->probe_ready = TRUE;

	return NULL;
}

// ProbeFifoReadFunction3:
// inlineable ProbeFifoRead Function -- similar to the thread, is called/inlined by/into the Image Data Thread
gpointer ProbeFifoReadFunction3 (void *ptr_sr, int dspdev){
	static int plotted=0;
	sranger_mk3_hwi_dev *sr = (sranger_mk3_hwi_dev*)ptr_sr;

	while (sr->is_scanning ()){
		switch (sr->ReadProbeFifo (dspdev)){
		case RET_FR_NOWAIT:
			plotted = 0;
			continue;
		case RET_FR_WAIT:
			if (DSPControlClass->probedata_length () > 0){
                                if (!plotted && g_settings_get_boolean (DSPControlClass->get_hwi_settings (), "probe-graph-enable-map-plot-events")){
                                        DSPControlClass->Probing_graph_update_thread_safe (0);
                                        plotted++;
                                }
                        }
			return NULL;
		case RET_FR_OK:
			plotted = 0;
			continue;
		case RET_FR_ERROR:
			goto error;
		case RET_FR_FCT_END: 
			plotted = 0;
			if (DSPControlClass->probedata_length () > 0){
                                if (g_settings_get_boolean (DSPControlClass->get_hwi_settings (), "probe-graph-enable-map-plot-events"))
                                        DSPControlClass->Probing_graph_update_thread_safe (1);

                                if (g_settings_get_boolean (DSPControlClass->get_hwi_settings (), "probe-graph-enable-map-save-events"))
					DSPControlClass->Probing_save_callback (NULL, DSPControlClass);

				DSPControlClass->push_probedata_arrays ();
				DSPControlClass->init_probedata_arrays ();
				LOGMSGS ( "ProbeFifoReadFunction -- Pushed Probe Data on Stack" << std::endl);
			}
			continue;
		}
		goto error;
	}
error:
	return NULL;
}

// ReadProbeFifo:
// read from probe FIFO, this engine needs to be called several times from master thread/function
int sranger_mk3_hwi_dev::ReadProbeFifo (int dspdev, int control){
	int pvd_blk_size=0;
	static double pv[9];
	static int last = 0;
	static int last_read_end = 0;
	static DATA_FIFO_EXTERN_PCOPY_MK3 fifo;
	static PROBE_SECTION_HEADER_MK3 section_header;
	static int next_header = 0;
	static int number_channels = 0;
	static int data_index = 0;
	static int end_read = 0;
	static int data_block_size=0;
	static int need_fct = FR_YES;  // need fifo control
	static int need_hdr = FR_YES;  // need header
	static int need_data = FR_YES; // need data
	static int ch_lut[32];
	static int ch_msk[]  = { 0x0000001, 0x0000002,   0x0000010, 0x0000020, 0x0000040, 0x0000080,   0x0000100, 0x0000200, 0x0000400, 0x0000800,
				 0x0000008, 0x0001000, 0x0002000, 0x0004000, 0x0008000,   0x0000004,   0x0000000 };
	static int ch_size[] = {    2, 2,    2, 2, 2, 2,   2, 2, 2, 2,    4,   4,  4,  4,   4,   4,  0 };
	static const char *ch_header[] = {"Zmon-AIC5Out", "Umon-AIC6Out", "AIC0-I", "AIC1", "AIC2", "AIC3", "AIC4", "AIC5", "AIC6", "AIC7",
					  "LockIn0", "LockIn1stA", "LockIn1stB", "LockIn2ndA", "LockIn2ndB", "Count", NULL };
	static short data[EXTERN_PROBEDATAFIFO_LENGTH];
	static double dataexpanded[16];
#ifdef LOGMSGS0
	static double dbg0=0., dbg1=0.;
	static int dbgi0=0;
#endif

	switch (control){
	case FR_FIFO_FORCE_RESET: // not used normally -- FIFO is reset by DSP at probe_init (single probe)
		fifo.r_position = 0;
		fifo.w_position = 0;
		check_and_swap (fifo.r_position);
		check_and_swap (fifo.w_position);
		lseek (dspdev, magic_data.probedatafifo, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
		sr_write (dspdev, &fifo, 2*sizeof(DSP_INT32)); // reset positions now to prevent reading old/last dataset before DSP starts init/putting data
		return RET_FR_OK;

	case FR_INIT:
		last = 0;
		next_header = 0;
		number_channels = 0;
		data_index = 0;
		last_read_end = 0;

		need_fct  = FR_YES;
		need_hdr  = FR_YES;
		need_data = FR_YES;

		DSPControlClass->init_probedata_arrays ();
		for (int i=0; i<16; dataexpanded[i++]=0.);

		LOGMSGS0 ( std::endl << "************** PROBE FIFO-READ INIT **************" << std::endl);
		LOGMSGS ( "FR::INIT-OK." << std::endl);
		return RET_FR_OK; // init OK.

	case FR_FINISH:
		LOGMSGS ( "FR::FINISH." << std::endl);
		return RET_FR_OK; // finish OK.

	default: break;
	}

	if (need_fct){ // read and check fifo control?

		LOGMSGS2 ( "FR::NEED_FCT, last: 0x"  << std::hex << last << std::dec << std::endl);

//		Seek for ATOMIC read
//		lseek (dspdev, magic_data.probedatafifo, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);

//		Seek for NON-ATOMIC read -- testing!!!
		lseek (dspdev, magic_data.probedatafifo, SRANGER_MK23_SEEK_DATA_SPACE);
		sr_read  (dspdev, &fifo, sizeof (fifo));
		check_and_swap (fifo.w_position);
		check_and_swap (fifo.current_section_head_position);
		check_and_swap (fifo.current_section_index);
		check_and_swap (fifo.p_buffer_base);
		end_read = fifo.w_position >= last ? fifo.w_position : EXTERN_PROBEDATAFIFO_LENGTH;

		LOGMSGS2 ( "FR::NEED_FCT, magic.prbdata_fifo  @Adr: 0x"  << std::hex << magic_data.probedatafifo << std::dec << std::endl);
		LOGMSGS2 ( "FR::NEED_FCT, magic.prbdata_fifo.p_buf: 0x"  << std::hex << fifo.p_buffer_base << std::dec << std::endl);
		LOGMSGS2 ( "FR::NEED_FCT, w_position: 0x"  << std::hex << fifo.w_position << std::dec << std::endl);

		// check for new data
		if ((end_read - last) < 1)
			return RET_FR_WAIT;
		else {
			need_fct  = FR_NO;
			need_data = FR_YES;
		}
	}

	if (need_data){ // read full FIFO block
		LOGMSGS1 ( "FR::NEED_DATA" << std::endl);

		int database = (int)fifo.p_buffer_base;
		int dataleft = end_read;
		int position = 0;
		if (fifo.w_position > last_read_end){
//			database += last_read_end;
//			dataleft -= last_read_end;
		}
		for (; dataleft > 0; database += 0x4000, dataleft -= 0x4000, position += 0x4000){
			LOGMSGS1 ( "FR::NEED_DATA: B::0x" <<  std::hex << database <<  std::dec << std::endl);
			//lseek (dspdev, database, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
			lseek (dspdev, database, SRANGER_MK23_SEEK_DATA_SPACE);
			sr_read  (dspdev, &data[position], (dataleft >= 0x4000 ? 0x4000 : dataleft)<<1);
		}
		last_read_end = end_read;
			
		need_data = FR_NO;
	}

	if (need_hdr){ // we have enough data if control gets here!
		LOGMSGS1 ( "FR::NEED_HDR" << std::endl);

		// check for FIFO loop
		if (last > (EXTERN_PROBEDATAFIFO_LENGTH - EXTERN_PROBEDATA_MAX_LEFT)){
			LOGMSGS ( "FR:FIFO LOOP DETECTED -- FR::NEED_HDR ** Data @ " 
				   << "0x" << std::hex << last
				   << " -2 :[" << (*((DSP_UINT32*)&data[last-2]))
				   << " " << (*((DSP_UINT32*)&data[last]))
				   << " " << (*((DSP_UINT32*)&data[last+2]))
				   << " " << (*((DSP_UINT32*)&data[last+4]))
				   << " " << (*((DSP_UINT32*)&data[last+6])) 
				   << "] : FIFO LOOP MARK " << std::dec << ( *((DSP_UINT32*)&data[last]) == 0 ? "OK":"ERROR")
				   << std::endl);
			next_header -= last;
			last = 0;
			end_read = fifo.w_position;
		}

		if (((fifo.w_position - last) < (sizeof (section_header)>>1))){
			need_fct  = FR_YES;
			return RET_FR_WAIT;
		}

		memcpy ((void*)&(section_header.srcs), (void*)(&data[next_header]), sizeof (section_header));
		check_and_swap (section_header.srcs);
		check_and_swap (section_header.n);
		check_and_swap (section_header.time);
		check_and_swap (section_header.x_offset);
		check_and_swap (section_header.y_offset);
		check_and_swap (section_header.z_offset);
		check_and_swap (section_header.x_scan);
		check_and_swap (section_header.y_scan);
		check_and_swap (section_header.z_scan);
		check_and_swap (section_header.u_scan);
		check_and_swap (section_header.section);
		
		// set vector of expanded data array representation, section start
		pv[0] = section_header.time;
		pv[1] = section_header.x_offset;
		pv[2] = section_header.y_offset;
		pv[3] = section_header.z_offset;
		pv[4] = section_header.x_scan;
		pv[5] = section_header.y_scan;
		pv[6] = section_header.z_scan;
		pv[7] = section_header.u_scan;
		pv[8] = section_header.section;

		LOGMSGS ( "FR::NEED_HDR -- got HDR @ 0x" << std::hex << next_header << std::dec 
			  << " section: " << section_header.section
			  << " time: " << section_header.time 
			  << " XYZ: " << section_header.x_scan << ", " << section_header.y_scan  << ", " << section_header.z_scan 
			  << std::endl);

		// validate header -- if stupid things are happening, values are messed up so check:
		if (section_header.time < 0 || section_header.section < 0 || section_header.section > 50 || section_header.n < 0 || section_header.n > 500000){
			LOGMSGS0 ( "************** FIFO READ ERROR DETECTED: bad section header **************" << std::endl);
			LOGMSGS0 ( "Error reading from ProbeData FIFO base[" << std::hex << fifo.p_buffer_base << std::dec << "]." << std::endl);
			LOGMSGS0 ( "==> read bad HDR @ 0x" << std::hex << next_header << std::dec 
				   << " last: 0x" << std::hex << last << std::dec 
				   << " section: " << section_header.section
				   << " n: " << section_header.n
				   << " time: " << section_header.time 
				   << " XYZ: " << section_header.x_scan << ", " << section_header.y_scan  << ", " << section_header.z_scan 
				   << std::endl);
			LOGMSGS0 ( "Last Sec: " << dbg0 << "Last srcs: " << dbg1 << " last last: 0x" << std::hex << dbgi0 << std::dec << std::endl);
			// baild out and try recovery
			goto auto_recover_and_debug;
		}

		DSPControlClass->add_probe_hdr (pv); // fwd port!

#ifdef LOGMSGS0
		dbg0 = section_header.section;
		dbg1 = section_header.srcs;
		dbgi0 = last;
#endif

		need_hdr = FR_NO;

		// analyze header and setup channel lookup table
		number_channels=0;
		last += sizeof (section_header) >> 1;
		next_header += sizeof (section_header) >> 1;

		data_block_size = 0;
		if (section_header.srcs){
			LOGMSGS ( "FR::NEED_HDR: decoding DATA_SRCS to read..." << std::endl);
			for (int i = 0; ch_msk[i] && i < 18; ++i){
				if (section_header.srcs & ch_msk[i]){
					ch_lut[number_channels] = i;
					data_block_size += ch_size[i];
					++number_channels;
				}
				if (i == 10 && (data_block_size>>1) & 1){ // adjust for even LONG position
					data_block_size += 2;
					ch_lut[number_channels] = -1; // dummy fill
					++number_channels;
				}
					    
			}
			next_header += section_header.n * (data_block_size >> 1);
			data_index = 0;

		} else {
			LOGMSGS ( "FR::NEED_HDR: DATA_SRCS ZERO." << std::endl);

			if (section_header.n == 0){
				LOGMSGS ( "FR::NEED_HDR: DATA_N ZERO -> END PROBE OK." << std::endl);
				DSPControlClass->probe_trigger_single_shot = 0; // if single shot, stop reading next
				number_channels = 0;
				data_index = 0;
				need_hdr = FR_YES;
				return RET_FR_FCT_END;
			} else {
				LOGMSGS ( "FR::NEED_HDR: no data in this section." << std::endl);
			}

			LOGMSGS ( "FR::NEED_HDR: need next hrd!" << std::endl);
			need_hdr = FR_YES;
			return RET_FR_NOWAIT;
		}
	}

	// now read/convert all available/valid data from fifo block we just copied
	pvd_blk_size = data_block_size >> 1; // data block size in "word"-indices
	LOGMSGS1 ( "FR::DATA-cvt:"
		  << " pvd_blk-sz="  << pvd_blk_size 
		  << " last=0x"      << std::hex << last << std::dec
		  << " next_header=0x" << std::hex << next_header << std::dec
		  << " end_read=0x"  << std::hex << end_read << " EPL:0x" << EXTERN_PROBEDATAFIFO_LENGTH << std::dec 
		  << " data_index="  << data_index << std::dec
		  << std::endl);

	for (int element=0; last < end_read; ++last, ++data_index){

		// got all data, at next header?
		if (last == next_header){
			LOGMSGS ( "FR:NEXT HDR EXPECTED" << std::endl);
			need_hdr = FR_YES;
			return RET_FR_NOWAIT;
		}
		if (last > next_header){
			LOGMSGS0 ( "FR:ERROR:: =====> MISSED NEXT HDR?"
				   << " last: 0x" << std::hex << last
				   << " next_header: 0x" << std::hex << next_header
				   << std::endl);
			goto auto_recover_and_debug;
		}

		int channel = ch_lut[element++];
		if (channel >= 0){ // only if valid (skip possible dummy (2) fillings)
			// check for long data (4)
			if (ch_size[channel] == 4){
				LNG *tmp = (LNG*)&data[last];
				check_and_swap (*tmp);
				dataexpanded[channel] = (double) (*tmp);
				++last; ++data_index;
			} else { // normal data (2)
				check_and_swap (data[last]);
				dataexpanded[channel] = (double) data[last];
			}
		}

		// add vector and data to expanded data array representation
		if ((data_index % pvd_blk_size) == (pvd_blk_size-1)){
			if (data_index >= pvd_blk_size) // skip to next vector
				DSPControlClass->add_probevector ();
			else // set vector as previously read from section header (pv[]) at sec start
				DSPControlClass->set_probevector (pv);
			// add full data vector
			DSPControlClass->add_probedata (dataexpanded);
			element = 0;

			// check for FIFO loop now
			if (last > (EXTERN_PROBEDATAFIFO_LENGTH - EXTERN_PROBEDATA_MAX_LEFT)){
				++last;
				LOGMSGS0 ( "FR:FIFO LOOP DETECTED ** Data @ " 
					   << "0x" << std::hex << last
					   << " last-6 :[" << (*((DSP_UINT32*)&data[last-6]))
					   << " " << (*((DSP_UINT32*)&data[last-4]))
					   << " " << (*((DSP_UINT32*)&data[last-2]))
					   << " " << (*((DSP_UINT32*)&data[last]))
					   << " " << (*((DSP_UINT32*)&data[last+2]))
					   << " " << (*((DSP_UINT32*)&data[last+4]))
					   << " " << (*((DSP_UINT32*)&data[last+6])) 
					   << "] : FIFO LOOP MARK " << std::dec << ( *((DSP_UINT32*)&data[last]) == 0 ? "OK":"ERROR")
					   << " FIFOLEN=" << fifo.length << std::endl);
				next_header -= last;
				last = -1; // compensate for ++last at of for(;;)!
				end_read = fifo.w_position;
			}
		}
	}

	LOGMSGS1 ( "FR:FIFO NEED FCT" << std::endl);
	need_fct = FR_YES;

	return RET_FR_WAIT;


// emergency bailout and auto recovery, FIFO restart
auto_recover_and_debug:

#ifdef LOGMSGS0
	LOGMSGS0 ( "************** -- FIFO DEBUG -- **************" << std::endl);
	LOGMSGS0 ( "LastArdess: " << "0x" << std::hex << last << std::dec << std::endl);
	for (int adr=last-8; adr < last+32; adr+=8){
		while (adr < 0) ++adr;
		LOGMSGS0 ("0x" << std::hex << adr << "::"
			  << " " << (*((DSP_UINT32*)&data[adr]))
			  << " " << (*((DSP_UINT32*)&data[adr+2]))
			  << " " << (*((DSP_UINT32*)&data[adr+4]))
			  << " " << (*((DSP_UINT32*)&data[adr+6]))
			  << std::dec << std::endl);
	}
	LOGMSGS0 ( "************** TRYING AUTO RECOVERY **************" << std::endl);
	LOGMSGS0 ( "***** -- STOP * RESET FIFO * START PROBE -- ******" << std::endl);
#endif

	DSPControlClass->dump_probe_hdr (); // TESTING

	// STOP PROBE
	// reset positions now to prevent reading old/last dataset before DSP starts init/putting data
	DSP_INT32 start_stop[2] = { 0, 1 };
	lseek (dspdev, magic_data.probe, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_write (dspdev, &start_stop, 2*sizeof(DSP_INT32));

	// RESET FIFO
	// reset positions now to prevent reading old/last dataset before DSP starts init/putting data
	fifo.r_position = 0;
	fifo.w_position = 0;
	check_and_swap (fifo.r_position);
	check_and_swap (fifo.w_position);
	lseek (dspdev, magic_data.probedatafifo, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_write (dspdev, &fifo, 2*sizeof(DSP_INT32));

	// START PROBE
	// reset positions now to prevent reading old/last dataset before DSP starts init/putting data
	start_stop[0] = 1;
	start_stop[1] = 0;
	lseek (dspdev, magic_data.probe, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_write (dspdev, &start_stop, 2*sizeof(DSP_INT32));

	// reset all internal and partial reinit FIFO read thread
	last = 0;
	next_header = 0;
	number_channels = 0;
	data_index = 0;
	last_read_end = 0;
	need_fct  = FR_YES;
	need_hdr  = FR_YES;
	need_data = FR_YES;

	// and start over on next call
	return RET_FR_WAIT;
}


/*
 * provide some info about the connected hardware/driver/etc.
 */
const gchar* sranger_mk3_hwi_dev::get_info(){
        static gchar *tmp=NULL;
	static gchar *magic_info = NULL;
        g_free (tmp);
	if (productid){
		gchar *details = NULL;
		if (FB_SPM_VERSION != magic_data.version){
			details = g_strdup_printf(
				"WARNING: Signal Ranger FB_SPM soft Version mismatch detected:\n"
				"Detected SRanger DSP Software Version: %x.%02x\n"
				"Detected SRanger DSP Software ID: %04X\n"
				"GXSM was build for DSP Software Version: %x.%02x\n"
				"And DSP Software ID %4X is recommended.\n"
				"This may cause incompatility problems, trying to proceed.\n",
				magic_data.version >> 8, 
				magic_data.version & 0xff,
				magic_data.dsp_soft_id,
				FB_SPM_VERSION >> 8, 
				FB_SPM_VERSION & 0xff,
				FB_SPM_SOFT_ID);
		} else {
			details = g_strdup_printf("Errors/Warnings: none\n");
		}
		g_free (magic_info); // g_free irgnors NULL ptr!
		magic_info = g_strdup_printf (
			"Magic....... : %08X\n"
			"Version..... : %08X\n"
			"Date........ : %08X%08X\n"
			"*-- DSP Control Struct Locations --*\n"
			"statemachine : %08x\n"
			"analog...... : %08x\n"
			"signal_mon.. : %08x\n"
			"feedback_mix.: %08x\n"
			"z_servo .... : %08x\n"
			"m_servo .... : %08x\n"
			"scan........ : %08x\n"
			"move........ : %08x\n"
			"probe....... : %08x\n"
			"autoapproach : %08x\n"
			"datafifo.... : %08x\n"
			"probedatafifo: %08x\n"
			"signal_lookup: %08x\n"
			"------------------------------------\n"
			"%s",
			magic_data.magic,
			magic_data.version,
			magic_data.year,
			magic_data.mmdd,
			magic_data.statemachine,
			magic_data.analog,
			magic_data.signal_monitor,
			magic_data.feedback_mixer,
			magic_data.z_servo,
			magic_data.m_servo,
			magic_data.scan,
			magic_data.move,
			magic_data.probe,
			magic_data.autoapproach,
			magic_data.datafifo,
			magic_data.probedatafifo,
			magic_data.signal_lookup,
			details
			);
		g_free (details);
		return tmp=g_strconcat (
                                        "*--GXSM Sranger HwI base class--*\n",
                                        "Sranger device: ", productid, "\n",
                                        "*--Features--*\n",
                                        FB_SPM_FEATURES,
                                        "*--Magic Info--*\n",
                                        magic_info,
                                        "*--EOF--*\n",
                                        NULL
                                        ); 
	}
	else
		return tmp=g_strdup("*--GXSM Sranger HwI base class--*\n"
                                    "Sranger device not connected\n"
                                    "or not supported.");
}

double sranger_mk3_hwi_dev::GetUserParam (gint n, gchar *id){
	return DSPControlClass->GetUserParam (n, id);
}

gint sranger_mk3_hwi_dev::SetUserParam (gint n, gchar *id, double value){
	return DSPControlClass->SetUserParam (n, id, value);
}







// ======================================== ========================================




#if 0
#define CONV_16_C(X) int_2_sranger_int (X)
#define CONV_32_C(X) long_2_sranger_long (X)

#define CONV_16(X) X = int_2_sranger_int (X)
#define CONV_32(X) X = long_2_sranger_long (X)

#else

#define CONV_16_C(X) (X)
#define CONV_32_C(X) (X)

#define CONV_16(X) 
#define CONV_32(X) 
#endif

void sranger_mk3_hwi_dev::read_dsp_state (gint32 &mode){
	lseek (dsp, magic_data.statemachine, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_read (dsp, &dsp_state, sizeof(dsp_state), "Rd-DSP-State"); 

	CONV_32 (dsp_state.mode);
	CONV_32 (dsp_state.DataProcessTime);
	CONV_32 (dsp_state.IdleTime);

	mode = dsp_state.mode;
}

void sranger_mk3_hwi_dev::write_dsp_state (gint32 mode){
        if (mode > 0){
	      dsp_state.set_mode = mode;
	      dsp_state.clr_mode = 0;
	}
        if (mode < 0){
	      dsp_state.set_mode = 0;
	      dsp_state.clr_mode = -mode;
	}
	CONV_32 (dsp_state.set_mode);
	CONV_32 (dsp_state.clr_mode);
	
	lseek (dsp, magic_data.statemachine, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_write (dsp, &dsp_state, 2*sizeof(gint32), "Adj-DSP-State"); 
}

// 	DSP_INT cp;           /* 0  0*< Const Proportional in Q15 rep =RW */
// 	DSP_INT ci;           /* 1  1*< Const Integral in Q15 rep =RW */
// 	DSP_INT setpoint;     /* 2  2*< real world setpoint, if loaded gain, spm_logn will be calculated and "soll" is updated =WO */
// 	DSP_INT ca_q15;       /* 3  3*< IIR filter adapt: ca    = q_sat_limit cut-off (0:=FBW or f0_max) [Q15] */
// 	DSP_LONG cb_Ic;       /* 4  4*< IIR filter adapt: cb_Ic = f_0,min/f_0,max * I_cross [32] */
// 	DSP_INT I_cross;      /* 5  6*< IIR filter adapt: I_crossover */
// 	DSP_INT I_offset;     /* 6  7*< I_offset, log start/characteristic */
// 	DSP_INT q_factor15;   /* 7  8*< IIR filter: q --  (In=1/(1+q) (I+q*In)) in Q15 */
// 	DSP_INT soll;         /* 8  9*< "Soll Wert" Set Point (behind log trafo) =RO */
// 	DSP_INT ist;          /* 9 10*< "Ist Wert" Real System Value (behind log trafo) =RO */
// 	DSP_INT delta;        /*10 11*< "Regelabweichung" ist-soll =RO */
// 	DSP_LONG i_pl_sum;    /*11 12*< CI - summing, includes area-scan plane-correction summing, 32bits! =RO */
// 	DSP_LONG i_sum;       /*12 14*< CI summing only, mirror, 32bits! =RO */
// 	DSP_LONG I_iir;       /*13 16*< Current I smooth (IIR response)  =RO */
// 	DSP_INT z;            /*14 18*< "Stellwert" Feedback Control Value "Z" =RO */
// 	DSP_INT I_fbw;        /*15 19*< I full Band width */
// 	DSP_INT zxx;          /*16 20*< spacer */
// 	DSP_INT zyy;          /*17 21*< spacer */
// 	DSP_LONG I_avg;       /*18 22*< not normalized, 256x */
// 	DSP_LONG I_rms;       /*19 24*< not normalized, 256x */
	
// DSP: FEEDBACK_MIXER
//	DSP_INT level[4]; /**< 0 if FUZZY MODE: Level for FUZZY interaction =RW */
//	DSP_INT gain[4];  /**< 4 Gain for signal in Q15 rep =RW */
//	DSP_INT mode[4];  /**< 8 mixer mode: 0=OFF -- Bit=1/0 for Bit0: Log/Lin, 1:IIR/FULL, 2:FUZZY/NORMAL */
//	DSP_INT setpoint[4]; /**< 12 individual setpoint for every signal */
//	DSP_INT src_lookup[4]; /**< 16 mixer input source lookups
void sranger_mk3_hwi_dev::conv_dsp_feedback (){
	CONV_32 (dsp_z_servo.cp);
	CONV_32 (dsp_z_servo.ci);
	CONV_32 (dsp_z_servo.setpoint);
	CONV_32 (dsp_z_servo.input);

	CONV_32 (dsp_m_servo.cp);
	CONV_32 (dsp_m_servo.ci);
	CONV_32 (dsp_m_servo.setpoint);
	CONV_32 (dsp_m_servo.input);

	for (int i=0; i<4; ++i){
		CONV_32 (dsp_feedback_mixer.level[i]);
		CONV_32 (dsp_feedback_mixer.gain[i]);
		CONV_32 (dsp_feedback_mixer.mode[i]);
		CONV_32 (dsp_feedback_mixer.setpoint[i]);
		CONV_32 (dsp_feedback_mixer.src_lookup[i]);
		CONV_32 (dsp_feedback_mixer.input_p[i]);
		CONV_32 (dsp_feedback_mixer.iir_ca_q15[i]);
		CONV_32 (dsp_feedback_mixer.iir_signal[i]); // **
	}
	CONV_32 (dsp_feedback_mixer.cb_Ic);
	CONV_32 (dsp_feedback_mixer.I_cross);
	CONV_32 (dsp_feedback_mixer.I_offset);
	CONV_32 (dsp_feedback_mixer.q_factor15);
	CONV_32 (dsp_feedback_mixer.delta); // **
}

double sranger_mk3_hwi_dev::read_dsp_feedback (const gchar *property, int index){
	lseek (dsp, magic_data.z_servo, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_read (dsp, &dsp_z_servo, sizeof (SERVO_MK3), "Rd-DSP-Z-Servo");

	lseek (dsp, magic_data.m_servo, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_read (dsp, &dsp_m_servo, sizeof (SERVO_MK3), "Rd-DSP-M-Servo");

	lseek (dsp, magic_data.feedback_mixer, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_read (dsp, &dsp_feedback_mixer, sizeof (FEEDBACK_MIXER_MK3), "Rd-DSP-FBMIX");   // read back..., "f0-now" (q_factor_15)

	// from DSP to PC -- only SetPoint!
	conv_dsp_feedback ();

        if (property){
                switch (property[0]){
                case 'M':
                        switch (property[1]){
                        case 'T': if (index>=0 && index<4) return (double)dsp_feedback_mixer.mode[index]; break;
                        case 'L': if (index>=0 && index<4) return (double)dsp_feedback_mixer.level[index]; break; // convert!
                        case 'G': if (index>=0 && index<4) return (double)dsp_feedback_mixer.gain[index]; break; // convert!
                        case 'S': if (index>=0 && index<4) return (double)dsp_feedback_mixer.setpoint[index]; break; // convert!
                        } break;
                }
                return 0.;
        } else return 0.;
}

void sranger_mk3_hwi_dev::write_dsp_feedback (
		      double set_point[4], double factor[4], double gain[4], double level[4], int transform_mode[4],
		      double IIR_I_crossover, double IIR_f0_max[4], double IIR_f0_min, double LOG_I_offset, int IIR_flag,
                      double setpoint_zpos, double z_servo[3], double m_servo[3], double pllref){

        SRANGER_DEBUG_SIG ( "---> sranger_mk3_hwi_dev::write_dsp_feedback --->");
	for (int i=0; i<4; ++i){
                
		if (i==0){ // TUNNEL-CURRENT dedicated channel
			dsp_feedback_mixer.setpoint[i] = (int)(round(256.*main_get_gapp()->xsm->Inst->VoltIn2Dig (main_get_gapp()->xsm->Inst->nAmpere2V (set_point[i])))); // Q23
                        dsp_feedback_mixer.level[i]    = (int)(round(256.*main_get_gapp()->xsm->Inst->VoltIn2Dig (factor[i]*level[i])));
                }else{
                        if (i==1){ // PLL-FREQ dedicated channel, INTERNAL PAC if Hertz2V is set to 0 in SPM settings -- 
                                if (main_get_gapp()->xsm->Inst->dHertz2V(1.) == 0.0){ // INTERNAL PAC/PLL
                                        if (pllref > 0.0) // use MK3 PAC-PLL
                                                dsp_feedback_mixer.setpoint[i] = (int)round((CPN(29)*2.*M_PI/150000.)*(set_point[i]+pllref)); // Q32 bit raw for PAC signal
                                        else{ // use McBSP based (RP) hi-speed PAC-PLL
                                                double skl = lookup_signal_scale_by_index (lookup_signal_by_name ("McBSP Freq"));
                                                dsp_feedback_mixer.setpoint[i] = (int)round(set_point[i]/skl); // Q32 bit raw for PAC signal
                                        }
                                        SRANGER_DEBUG_SIG (
                                                           "MIX[1]:"
                                                           << " pllref   = " << pllref << "Hz"
                                                           << " setpoint = " << set_point[i] << "Hz"
                                                           << " PLL setpoint absolute = " << (set_point[i]+pllref) << "Hz"
                                                           << " DSP-setpoint[1] = " << dsp_feedback_mixer.setpoint[i]
                                                           );
                                } else {
                                        dsp_feedback_mixer.setpoint[i] = (int)round(256.*round(main_get_gapp()->xsm->Inst->VoltIn2Dig (main_get_gapp()->xsm->Inst->dHertz2V (set_point[i])))); // Q23
                                        SRANGER_DEBUG_SIG (
                                                           "MIX[1]:"
                                                           << " dHz2Volt = " << main_get_gapp()->xsm->Inst->dHertz2V(1.) << "Hz/Volt"
                                                           << " setpoint = " << set_point[i] << "Hz"
                                                           << " setpoint volt   = " << main_get_gapp()->xsm->Inst->dHertz2V (set_point[i]) << "V"
                                                           << " DSP-setpoint[1] = " << dsp_feedback_mixer.setpoint[i] 
                                                           );
                                }
                                dsp_feedback_mixer.level[i]    = (int)(round(256.*main_get_gapp()->xsm->Inst->VoltIn2Dig (factor[i]*level[i])));
                        } else {
                                // general purpose In-N signal "Volt" or accordign to dsp_signal table
                                //dsp_feedback_mixer.setpoint[i] = (int)(round(256.*main_get_gapp()->xsm->Inst->VoltIn2Dig (factor[i]*set_point[i]))); // In0-7 Q23
                                dsp_feedback_mixer.setpoint[i] = (int)(round(factor[i]*set_point[i])); // native signal (32bit)
                                dsp_feedback_mixer.level[i]    = (int)(round(factor[i]*level[i]));
                        }
                }
                //dsp_feedback_mixer.level[i]    = (int)(round(256.*main_get_gapp()->xsm->Inst->VoltIn2Dig (factor[i]*level[i])));
		dsp_feedback_mixer.gain[i]     = float_2_sranger_q15 (gain[i]);
		dsp_feedback_mixer.mode[i]     = transform_mode[i];
		dsp_feedback_mixer.iir_ca_q15[i] = float_2_sranger_q15 (exp (-2.*M_PI*IIR_f0_max[i]/75000.));
		dsp_feedback_mixer.Z_setpoint   = (int)(round((1<<16)*main_get_gapp()->xsm->Inst->ZA2Dig (setpoint_zpos))); // new ZPos reference for FUZZY-LOG CONST HEIGHT MODE
                SRANGER_DEBUG_SIG (
                               " DSP-setpoint[" << i << "] = " << dsp_feedback_mixer.setpoint[i]
                               );
                SRANGER_DEBUG_SIG (
                               "IIR["<<i<<"]: " << dsp_feedback_mixer.iir_ca_q15[i]
                               );
	}

	// IIR self adaptive filter parameters for MIX0 channel, MIX1..3 IIR only -- assuming current
	double ca,cb;
	double Ic = main_get_gapp()->xsm->Inst->VoltIn2Dig (main_get_gapp()->xsm->Inst->nAmpere2V (1e-3*IIR_I_crossover)); // given in pA
	ca = exp (-2.*M_PI*IIR_f0_max[0]/75000.);

	dsp_feedback_mixer.cb_Ic    = (DSP_INT32) (32767. * (cb = IIR_f0_min/IIR_f0_max[0] * Ic));
	dsp_feedback_mixer.I_cross  = (int)Ic; 
	dsp_feedback_mixer.I_offset = (int)(round(256.*main_get_gapp()->xsm->Inst->VoltIn2Dig (main_get_gapp()->xsm->Inst->nAmpere2V (1e-3*LOG_I_offset)))); // given in pA -- MK3: Q23

	if (!IIR_flag)
		dsp_feedback_mixer.I_cross = 0; // disable IIR!

#if 0
	if (!IIR_flag)
		std::cout << "IIR: off" << std::endl;
	else {
		std::cout << "IIR: ca'    Q15= " << dsp_feedback_mixer.ca_q15 << " := " << ca << std::endl;
		std::cout << "IIR: cb_Ic' Q31= " << dsp_feedback_mixer.cb_Ic  << " := " << cb << std::endl;
		std::cout << "IIR: Ic        = " << dsp_feedback_mixer.I_cross  << std::endl;
		std::cout << "IIR: Io        = " << dsp_feedback_mixer.I_offset << std::endl;
	}
#endif


	// Z-Servo --- master feedback settings
	// Scale Regler Consts. with 1/VZ and convert to SR-Q15
	
	// NOTE:  CP, CI-DSP max at gain "1" = 1 in Q32 -- arb. div/100 for max assumed gain of 100.
	dsp_z_servo.cp = float_2_sranger_q31 (0.01 * z_servo[SERVO_CP] / sranger_mk2_hwi_pi.app->xsm->Inst->VZ ());
	dsp_z_servo.ci = float_2_sranger_q31 (0.01 * z_servo[SERVO_CI] / sranger_mk2_hwi_pi.app->xsm->Inst->VZ ());

	// Motor Servo:
#if 1
	dsp_m_servo.setpoint = float_2_sranger_q31 (main_get_gapp()->xsm->Inst->VoltIn2Dig (m_servo[0]));
#else
#define McBSP_FREQ_CONVERSION (125000000./17592186044415.) // 125MHz / ((1<<44)-1)
        g_print("M Servo McBSPConv: Frq Set=%g  %d\n", m_servo[0], (gint32)(m_servo[0]/McBSP_FREQ_CONVERSION));
	dsp_m_servo.setpoint = (gint32)(m_servo[0]/McBSP_FREQ_CONVERSION);
#endif
	dsp_m_servo.cp = float_2_sranger_q31 (0.01 * m_servo[SERVO_CP]);
	dsp_m_servo.ci = float_2_sranger_q31 (0.01 * m_servo[SERVO_CI]);


	// from PC to DSP
	conv_dsp_feedback ();

	lseek (dsp, magic_data.z_servo, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_write (dsp, &dsp_z_servo, MAX_WRITE_SERVO<<1); 

	lseek (dsp, magic_data.m_servo, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_write (dsp, &dsp_m_servo, MAX_WRITE_SERVO<<1); 

	lseek (dsp, magic_data.feedback_mixer, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_write (dsp, &dsp_feedback_mixer, MAX_WRITE_FEEDBACK_MIXER<<1);

	// from DSP to PC
	conv_dsp_feedback ();

        SRANGER_DEBUG_SIG ( "<--- sranger_mk3_hwi_dev::write_dsp_feedback <--- done.");
}

void sranger_mk3_hwi_dev::recalculate_dsp_scan_speed_parameters () {
	double tmp;
	DSPControlClass->recalculate_dsp_scan_speed_parameters (tmp_dnx, tmp_dny, tmp_fs_dx, tmp_fs_dy, tmp_nx_pre, tmp_fast_return);
	
	PI_DEBUG_GP (DBG_L4, "*** recalculated DSP scan speed parameters [MK3-Pro] ***\n");
	PI_DEBUG_GP (DBG_L4, "sizeof dsp_scan   %d B\n", (int) sizeof (dsp_scan));
	PI_DEBUG_GP (DBG_L4, "sizeof scan.fs_dx %d B\n", (int) sizeof (dsp_scan.fs_dx));

	dsp_scan.fs_dx  = (DSP_INT32)tmp_fs_dx;
	dsp_scan.fs_dy  = (DSP_INT32)tmp_fs_dy;
	dsp_scan.dnx    = (DSP_INT32)tmp_dnx;
	dsp_scan.dny    = (DSP_INT32)tmp_dny;
	dsp_scan.nx_pre = (DSP_INT32)tmp_nx_pre;
	dsp_scan.fast_return = (DSP_INT32)tmp_fast_return;
	tmp = 1000. + 3. * ( fabs ((double)dsp_scan.fs_dx*(double)dsp_scan.fm_dz0_xy_vec[i_X]) + fabs((double)dsp_scan.fs_dy*(double)dsp_scan.fm_dz0_xy_vec[i_Y]) )/2147483648.; // =1<<31
	PI_DEBUG_GP (DBG_L4, "Z-Slope-Max 3x (real): %g \n", tmp);
	if (tmp > 67108864.)
		tmp = 67108864.; // 1<<26
	PI_DEBUG_GP (DBG_L4, "Z-Slope-Max (real, lim): %g \n", tmp);
	dsp_scan.z_slope_max = (DSP_INT32)(tmp);
	
	dsp_scan.Zoff_2nd_xp = tmp_sRe = 0;
	dsp_scan.Zoff_2nd_xm = tmp_sIm = 0;
	if (tmp_fast_return == -1){ // -1: indicates fast scan mode with X sin actuation
#define CPN(N) ((double)(1LL<<(N))-1.)
		double deltasRe, deltasIm;
		double Num = (double)dsp_scan.dnx * (double)Nx * 2. + (double)dsp_scan.dny + (double) dsp_scan.nx_pre;
		// calculate Sine generator parameter
		// -- here 2147483647 * cos (2pi/N), N=n*dnx*2+dny   **** cos (2.*M_PI*freq[0]/150000.))
		deltasRe = round (CPN(31) * cos (2.*M_PI/Num));
		deltasIm = round (CPN(31) * sin (2.*M_PI/Num));
		dsp_scan.Zoff_2nd_xp = tmp_sRe = (int)deltasRe; // shared variables, no 2nd line mode in fast scan!
		dsp_scan.Zoff_2nd_xm = tmp_sIm = (int)deltasIm;
		PI_DEBUG_GP (DBG_L4, "DNX*NX*2+DNY......** %12.2f \n", Num);
		PI_DEBUG_GP (DBG_L4, "deltasRe..........** %d \n",dsp_scan.Zoff_2nd_xp);
		PI_DEBUG_GP (DBG_L4, "deltasIm..........** %d \n",dsp_scan.Zoff_2nd_xm);
	}

	PI_DEBUG_GP (DBG_L4, "x,y slope....** %d, %d \n", dsp_scan.fm_dz0_xy_vec[i_X], dsp_scan.fm_dz0_xy_vec[i_Y]);
	PI_DEBUG_GP (DBG_L4, "z_slope_max..** %d \n", dsp_scan.z_slope_max);
	PI_DEBUG_GP (DBG_L4, "FSdX.........** %d \n", dsp_scan.fs_dx);
	PI_DEBUG_GP (DBG_L4, "FSdY.........** %d \n", dsp_scan.fs_dy);
	PI_DEBUG_GP (DBG_L4, "DNX..........** %d \n", dsp_scan.dnx);
	PI_DEBUG_GP (DBG_L4, "DNY..........** %d \n", dsp_scan.dny);
	PI_DEBUG_GP (DBG_L4, "NX...........** %d \n", (int)Nx);
	PI_DEBUG_GP (DBG_L4, "NY...........** %d \n", (int)Ny);
	PI_DEBUG_GP (DBG_L4, "NXPre........** %d \n", dsp_scan.nx_pre);

}

void sranger_mk3_hwi_dev::recalculate_dsp_scan_slope_parameters () {
	double tmp;
	double swx = (double)dsp_scan.fs_dx * (double)dsp_scan.dnx * (double)dsp_scan.nx / 4294967296.; // =1<<32
	double swy = (double)dsp_scan.fs_dy * (double)dsp_scan.dny * (double)dsp_scan.ny / 4294967296.; // =1<<32
	DSPControlClass->recalculate_dsp_scan_slope_parameters (dsp_scan.fs_dx, dsp_scan.fs_dy,	
								dsp_scan.fm_dz0_xy_vec[i_X], dsp_scan.fm_dz0_xy_vec[i_Y],
								swx, swy);

	PI_DEBUG_GP (DBG_L4, "*** recalculated DSP slope parameters ***\n");

	PI_DEBUG_GP (DBG_L4, "FM_dz0x......** %d\n", dsp_scan.fm_dz0_xy_vec[i_X]);
	PI_DEBUG_GP (DBG_L4, "FM_dz0y......** %d\n", dsp_scan.fm_dz0_xy_vec[i_Y]);

	tmp = 1000. + 3. * ( fabs ((double)dsp_scan.fs_dx*(double)dsp_scan.fm_dz0_xy_vec[i_X]) + fabs((double)dsp_scan.fs_dy*(double)dsp_scan.fm_dz0_xy_vec[i_Y]) )/2147483648.; // =1<<31
	PI_DEBUG_GP (DBG_L4, "Z-Slope-Max 3x (real): %g \n", tmp);
	if (tmp > 67108864.)
		tmp = 67108864.; // 1<<26
	PI_DEBUG_GP (DBG_L4, "Z-Slope-Max (real, lim): %g \n", tmp);
	dsp_scan.z_slope_max = (DSP_INT32)(tmp);

}

void sranger_mk3_hwi_dev::conv_dsp_analog (){
	CONV_32 (dsp_analog.bias[0]);
	CONV_32 (dsp_analog.bias[1]);
	CONV_32 (dsp_analog.bias[2]);
	CONV_32 (dsp_analog.bias[3]);
	CONV_32 (dsp_analog.motor);
}

void sranger_mk3_hwi_dev::read_dsp_analog (){
	// read all analog
	lseek (dsp, magic_data.analog, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_read  (dsp, &dsp_analog, sizeof (dsp_analog), "Rd-DSP-ANALOG"); 

	// from DSP to PC
	conv_dsp_analog ();
}

void sranger_mk3_hwi_dev::write_dsp_analog (double bias[4], double motor){
	read_dsp_analog ();

	dsp_analog.bias[0]  = (gint32)round (CONST_DSP_F16*main_get_gapp()->xsm->Inst->VoltOut2Dig (main_get_gapp()->xsm->Inst->BiasV2Vabs (bias[0])));
	dsp_analog.bias[1]  = (gint32)round (CONST_DSP_F16*main_get_gapp()->xsm->Inst->VoltOut2Dig (main_get_gapp()->xsm->Inst->BiasV2Vabs (bias[1])));
	dsp_analog.bias[2]  = (gint32)round (CONST_DSP_F16*main_get_gapp()->xsm->Inst->VoltOut2Dig (main_get_gapp()->xsm->Inst->BiasV2Vabs (bias[2])));
	dsp_analog.bias[3]  = (gint32)round (CONST_DSP_F16*main_get_gapp()->xsm->Inst->VoltOut2Dig (main_get_gapp()->xsm->Inst->BiasV2Vabs (bias[3])));
	dsp_analog.motor = (gint32)round (CONST_DSP_F16*main_get_gapp()->xsm->Inst->VoltOut2Dig (main_get_gapp()->xsm->Inst->BiasV2Vabs (motor)));

	// only "bias" and "motor" is touched here!

	// from PC to DSP
	conv_dsp_analog ();

	lseek (dsp, magic_data.analog, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_write (dsp, &dsp_analog, MAX_WRITE_ANALOG_CONTROL<<1); 

	// from DSP to PC
	conv_dsp_analog ();
}

void sranger_mk3_hwi_dev::conv_dsp_scan (){
        CONV_32 (dsp_scan.rotm[0]);
        CONV_32 (dsp_scan.rotm[1]);
        CONV_32 (dsp_scan.rotm[2]);
        CONV_32 (dsp_scan.rotm[3]);
        CONV_32 (dsp_scan.fast_return);
        CONV_32 (dsp_scan.z_slope_max);
        CONV_32 (dsp_scan.nx_pre);
        CONV_32 (dsp_scan.dnx_probe);
        CONV_32 (dsp_scan.raster_a);
        CONV_32 (dsp_scan.raster_b);
        CONV_32 (dsp_scan.srcs_xp);
        CONV_32 (dsp_scan.srcs_xm);
        CONV_32 (dsp_scan.srcs_2nd_xp);
        CONV_32 (dsp_scan.srcs_2nd_xm);
        CONV_32 (dsp_scan.nx);
        CONV_32 (dsp_scan.ny);
        CONV_32 (dsp_scan.fs_dx);
        CONV_32 (dsp_scan.fs_dy);
        CONV_32 (dsp_scan.num_steps_move_xy);
        CONV_32 (dsp_scan.fm_dx);
        CONV_32 (dsp_scan.fm_dy); 
        CONV_32 (dsp_scan.fm_dzxy);
        CONV_32 (dsp_scan.fm_dz0_xy_vec[i_X]);
        CONV_32 (dsp_scan.fm_dz0_xy_vec[i_Y]);
        CONV_32 (dsp_scan.dnx);
        CONV_32 (dsp_scan.dny);
        CONV_32 (dsp_scan.Zoff_2nd_xp);
        CONV_32 (dsp_scan.Zoff_2nd_xm);
        CONV_32 (dsp_scan.xyz_vec[i_X]);
        CONV_32 (dsp_scan.xyz_vec[i_Y]);
        CONV_32 (dsp_scan.cfs_dx);
        CONV_32 (dsp_scan.cfs_dy);
	// gain mirrors
        CONV_32 (dsp_scan.xyz_gain);
        CONV_32 (dsp_move.xyz_gain);
// RO:
        CONV_32 (dsp_scan.sstate);
        CONV_32 (dsp_scan.pflg);
}

void sranger_mk3_hwi_dev::read_dsp_scan (gint32 &pflg){
	lseek (dsp, magic_data.scan, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_read  (dsp, &dsp_scan, sizeof (dsp_scan));

	// from DSP to PC
	conv_dsp_scan ();

	pflg = dsp_scan.pflg;
}

void sranger_mk3_hwi_dev::write_dsp_scan (){
	// from PC to DSP
	conv_dsp_scan ();

	// adjust ScanParams/Speed...
	lseek (dsp, magic_data.scan, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_write (dsp, &dsp_scan, MAX_WRITE_SCAN<<1);

	// from DSP to PC
	conv_dsp_scan ();
}

void sranger_mk3_hwi_dev::conv_dsp_probe (){
	CONV_32 (dsp_probe.start);
	//	CONV_32 (dsp_probe.LIM_up);
	//	CONV_32 (dsp_probe.LIM_dn);
	CONV_32 (dsp_probe.noise_amp);
	CONV_32 (dsp_probe.AC_amp[0]);
	CONV_32 (dsp_probe.AC_amp[1]);
	CONV_32 (dsp_probe.AC_amp[2]);
	CONV_32 (dsp_probe.AC_amp[3]);
	CONV_32 (dsp_probe.AC_amp_aux);
	CONV_32 (dsp_probe.AC_frq);
	CONV_32 (dsp_probe.AC_phaseA);
	CONV_32 (dsp_probe.AC_phaseB);
	CONV_32 (dsp_probe.AC_nAve);
	CONV_32 (dsp_probe.state);
	CONV_32 (dsp_probe.lockin_shr_corrprod);
	CONV_32 (dsp_probe.lockin_shr_corrsum);
//	CONV_32 (dsp_probe.);
	CONV_32 (dsp_probe.vector_head);

}
	
void sranger_mk3_hwi_dev::read_dsp_lockin (double AC_amp[4], double &AC_frq, double &AC_phaseA, double &AC_phaseB, gint32 &AC_lockin_avg_cycels){
	lseek (dsp, magic_data.probe, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_read (dsp, &dsp_probe, sizeof (dsp_probe)); 

	// from DSP to PC
	conv_dsp_probe ();

        SRANGER_DEBUG_SIG (
                       "sranger_mk3_hwi_dev::read_dsp_lockin: LockIn SHR Settings: corrprod>>:" << dsp_probe.lockin_shr_corrprod << " corrsum>>:" << dsp_probe.lockin_shr_corrprod
                       );

	// update, reconvert
	AC_amp[0] = main_get_gapp()->xsm->Inst->Dig2VoltOut ((double)dsp_probe.AC_amp) * main_get_gapp()->xsm->Inst->BiasGainV2V ();
	AC_amp[1] = main_get_gapp()->xsm->Inst->Dig2ZA (dsp_probe.AC_amp_aux);
	AC_frq = dsp_probe.AC_frq;
	if (AC_frq < 32)
 		AC_frq = 150000./512.*AC_frq;

	AC_phaseA = (double)dsp_probe.AC_phaseA/16.;
	AC_phaseB = (double)dsp_probe.AC_phaseB/16.;
	AC_lockin_avg_cycels = dsp_probe.AC_nAve;
        SRANGER_DEBUG_SIG ( "LockIn SHR Settings read back.");
}

int sranger_mk3_hwi_dev::dsp_lockin_state(int set){
	lseek (dsp, magic_data.probe, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_read (dsp, &dsp_probe, sizeof (dsp_probe)); 
	dsp_probe.start = 0;
	dsp_probe.stop  = 0;
	switch (set){
	case 0:	dsp_probe.stop = PROBE_RUN_LOCKIN_FREE;
		CONV_32 (dsp_probe.start);
		CONV_32 (dsp_probe.stop);
		lseek (dsp, magic_data.probe, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
		sr_write (dsp, &dsp_probe,  MAX_WRITE_PROBE<<1); 
		return 0;
		break;
	case 1:	dsp_probe.start = PROBE_RUN_LOCKIN_FREE;
		CONV_32 (dsp_probe.start);
		CONV_32 (dsp_probe.stop);
		lseek (dsp, magic_data.probe, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
		sr_write (dsp, &dsp_probe,  MAX_WRITE_PROBE<<1); 
		return 0;
		break;
	case -1:
		CONV_32 (dsp_probe.state);
		return dsp_probe.state == PROBE_RUN_LOCKIN_FREE ? 1:0;
		break;
	}
	return -1;
}

void sranger_mk3_hwi_dev::write_dsp_lockin_probe_final (double AC_amp[4], double &AC_frq, double AC_phaseA, double AC_phaseB, gint32 AC_lockin_avg_cycels, double VP_lim_val[2],  double noise_amp, int start) {
	double lvs=1.;
        SRANGER_DEBUG_SIG (
                       "sranger_mk3_hwi_dev::write_dsp_lockin_probe_final"
                       );
	// update all from DSP
	lseek (dsp, magic_data.probe, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_read (dsp, &dsp_probe, sizeof (dsp_probe));

	// from DSP to PC
	conv_dsp_probe ();
	
	// update with changed user parameters
	dsp_probe.start = start;
	// check for signal scale Q32s16.15, or Q32s24.8

        SRANGER_DEBUG_SIG (
                       "VECPROBE_LIMITER on     : "
                       << dsp_signal_lookup_managed[query_module_signal_input(DSP_SIGNAL_VECPROBE_LIMITER_INPUT_ID)].label
                       );
        SRANGER_DEBUG_SIG (
                       "VECPROBE_LIMITER unit   : (" << VP_lim_val[0] << ", " << VP_lim_val[1] << ") "
                       << dsp_signal_lookup_managed[query_module_signal_input(DSP_SIGNAL_VECPROBE_LIMITER_INPUT_ID)].unit
                       );
        SRANGER_DEBUG_SIG (
                       "VECPROBE_LIMITER 1/scale: " << (lvs = 1./dsp_signal_lookup_managed[query_module_signal_input(DSP_SIGNAL_VECPROBE_LIMITER_INPUT_ID)].scale)
                       );
        SRANGER_DEBUG_SIG (
                       "VECPROBE_LIMITER up,dn  : (" << (VP_lim_val[0]*lvs) << ", " << (VP_lim_val[1]*lvs) << ")"
                       );

	//	dsp_probe.LIM_up = (gint32)round (CONST_DSP_F16*main_get_gapp()->xsm->Inst->VoltOut2Dig (VP_lim_val));
	//	dsp_probe.LIM_dn = (gint32)round (CONST_DSP_F16*main_get_gapp()->xsm->Inst->VoltOut2Dig (VP_lim_val));

	// ==> setup signal, point to user var, setup user var
	//	DSP_INT32_P   limiter_updn[2]; /**<: limiter value signal upper, lower =WR */
        // ==> use
        //         usrarr_signal = self.lookup_signal_by_name ("user signal array")
        //         self.write_parameter (usrarr[SIG_ADR], struck.pack ("<ll", int (probe_LIM_up), int (probe_LIM_dn)))
 

	dsp_probe.AC_amp = DVOLT2AIC (AC_amp[0]);
	dsp_probe.AC_amp_aux = main_get_gapp()->xsm->Inst->ZA2Dig (AC_amp[1]);
	dsp_probe.AC_frq = (int) (AC_frq); // DSP selects dicrete AC Freq. out of 75000/(128/N) N=1,2,4,8
	dsp_probe.AC_phaseA = (int)round(AC_phaseA*16.);
	dsp_probe.AC_phaseB = (int)round(AC_phaseB*16.);
	dsp_probe.AC_nAve = AC_lockin_avg_cycels;
	if (AC_amp[2] >= 0. && AC_amp[2] <= 32. && AC_amp[3] >= -32. && AC_amp[3] <= 32.){
	        dsp_probe.lockin_shr_corrprod = (int)AC_amp[2];
		dsp_probe.lockin_shr_corrsum  = (int)AC_amp[3];
                SRANGER_DEBUG_SIG (
                               "sranger_mk3_hwi_dev::write_dsp_lockin_probe_final: LockIn SHR Settings: corrprod>>:"
                               << dsp_probe.lockin_shr_corrprod
                               << " corrsum>>:"
                               << dsp_probe.lockin_shr_corrsum
                               );
	} else { // safe defaults
	        dsp_probe.lockin_shr_corrprod = 14;
		dsp_probe.lockin_shr_corrsum  = 0;
                SRANGER_DEBUG_SIG (
                               "LockIn SHR DEFAULT Settings: corrprod>>:"
                               << dsp_probe.lockin_shr_corrprod
                               << " corrsum>>:"
                               << dsp_probe.lockin_shr_corrsum
                               );
	}
        SRANGER_DEBUG_SIG ( "sranger_mk3_hwi_dev::write_dsp_lockin_probe_final: XFrq");
	// update AC_frq to actual
	// AIC samples at 150000Hz, full Sintab length is 512, so need increments of
	// 1,2,4,8 for the following discrete frequencies
	const int AC_TAB_INC = 4;
	int AC_tab_inc=0;
	if (AC_frq >= 8000)
		AC_tab_inc = AC_TAB_INC<<3;     // LockIn-Ref on 9375.00Hz
	else if (AC_frq >= 4000)
		AC_tab_inc = AC_TAB_INC<<2;     // LockIn-Ref on 4687.50Hz
	else if (AC_frq >= 2000)
		AC_tab_inc = AC_TAB_INC<<1;     // LockIn-Ref on 2343.75Hz
	else if (AC_frq >= 1000)
		AC_tab_inc = AC_TAB_INC;        // LockIn-Ref on 1171.88Hz
	else if (AC_frq >= 500)
		AC_tab_inc = AC_TAB_INC>>1;     // LockIn-Ref on  585.94Hz
	else    AC_tab_inc = AC_TAB_INC>>2;     // LockIn-Ref on  292.97Hz
	
	AC_frq = 150000./512.*AC_tab_inc;

	dsp_probe.noise_amp = DVOLT2AIC (noise_amp);


	//##	for (int i=0; i<4; ++i){
	//		dsp_probe.src_lookup[i] = src_lookup[i];
	//		dsp_probe.prb_lookup[i] = prb_lookup[i];
	//		dsp_probe.LockIn_lookup[i] = lockin_lookup[i];
	//	}

	// from PC to DSP
	conv_dsp_probe ();
	lseek (dsp, magic_data.probe, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_write (dsp, &dsp_probe,  MAX_WRITE_PROBE<<1); 
	// from DSP to PC
	conv_dsp_probe ();
        SRANGER_DEBUG_SIG ( "sranger_mk3_hwi_dev::write_dsp_lockin_probe_final done.");
}


void sranger_mk3_hwi_dev::conv_dsp_vector (){
	CONV_32 (dsp_vector.n);
	CONV_32 (dsp_vector.dnx);  
	CONV_32 (dsp_vector.srcs);
	CONV_32 (dsp_vector.options);
	CONV_32 (dsp_vector.ptr_fb);
	CONV_32 (dsp_vector.repetitions);
	CONV_32 (dsp_vector.i);
	CONV_32 (dsp_vector.j);
	CONV_32 (dsp_vector.ptr_next);
	CONV_32 (dsp_vector.ptr_final);
	CONV_32 (dsp_vector.f_du);
	CONV_32 (dsp_vector.f_dx);
	CONV_32 (dsp_vector.f_dy);
	CONV_32 (dsp_vector.f_dz);
	CONV_32 (dsp_vector.f_dx0);
	CONV_32 (dsp_vector.f_dy0);
	CONV_32 (dsp_vector.f_dphi);
}

// PZ may need t oadjust...
#define EXTERN_PROBE_VECTOR_HEAD_DEFAULT 0x10F14000

void sranger_mk3_hwi_dev::write_dsp_vector (int index, PROBE_VECTOR_GENERIC *__dsp_vector){
	// update all from DSP
	lseek (dsp, magic_data.probe, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_read (dsp, &dsp_probe, sizeof (dsp_probe)); 
	// from DSP to PC
	conv_dsp_probe ();

	if (dsp_probe.vector_head < EXTERN_PROBE_VECTOR_HEAD_DEFAULT || index > 40 || index < 0){
		sranger_mk2_hwi_pi.app->message("Error writing Probe Vector:\n Bad vector address, aborting request.");
		return;
	}

	// copy/convert to DSP format
	dsp_vector.n = __dsp_vector->n;
	dsp_vector.dnx = __dsp_vector->dnx;
	dsp_vector.srcs = __dsp_vector->srcs;
	dsp_vector.options = __dsp_vector->options;
	dsp_vector.ptr_fb = __dsp_vector->ptr_fb;
	dsp_vector.repetitions = __dsp_vector->repetitions;
	dsp_vector.i = __dsp_vector->i;
	dsp_vector.j = __dsp_vector->j;
	dsp_vector.ptr_next = __dsp_vector->ptr_next;
	dsp_vector.ptr_final = __dsp_vector->ptr_final;
	dsp_vector.f_du = __dsp_vector->f_du;
	dsp_vector.f_dx = __dsp_vector->f_dx;
	dsp_vector.f_dy = __dsp_vector->f_dy;
	dsp_vector.f_dz = __dsp_vector->f_dz;
	dsp_vector.f_dx0 = __dsp_vector->f_dx0;
	dsp_vector.f_dy0 = __dsp_vector->f_dy0;
	dsp_vector.f_dphi = __dsp_vector->f_dphi;


	// check count ranges
	// NULL VECTOR, OK: END
	if (!(dsp_vector.n == 0 && dsp_vector.dnx == 0 && dsp_vector.ptr_next == 0 && dsp_vector.ptr_final == 0))
                if (dsp_vector.dnx < 0 || dsp_vector.dnx > 32767 || dsp_vector.n <= 0 || dsp_vector.n > 32767){
                        gchar *msg = g_strdup_printf ("Probe Vector [pc%02d] not acceptable:\n"
                                                      "n   = %6d   [0..32767]\n"
                                                      "dnx = %6d   [0..32767]\n"
                                                      "Auto adjusting.\n"
                                                      "Hint: adjust slope/speed/points/time.",
                                                      index, dsp_vector.n, dsp_vector.dnx );
                        main_get_gapp()->warning (msg);
                        g_free (msg);
                        if (dsp_vector.dnx < 0) dsp_vector.dnx = 0;
                        if (dsp_vector.dnx > 32767) dsp_vector.dnx = 32767;
                        if (dsp_vector.n <= 0) dsp_vector.n = 1;
                        if (dsp_vector.n > 32767) dsp_vector.n = 32767;
                }

	// setup VPC essentials
	dsp_vector.i = dsp_vector.repetitions; // preload repetitions counter now! (if now already done)
	dsp_vector.j = 0; // not yet used -- XXXX

	// from PC to to DSP format
        conv_dsp_vector();
	lseek (dsp, dsp_probe.vector_head + ((index*SIZE_OF_PROBE_VECTOR)<<1), SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_write (dsp, &dsp_vector, SIZE_OF_PROBE_VECTOR<<1);

	// from DSP to PC
        conv_dsp_vector();
}

void sranger_mk3_hwi_dev::read_dsp_vector (int index, PROBE_VECTOR_GENERIC *__dsp_vector){

	// update all from DSP
	lseek (dsp, magic_data.probe, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_read (dsp, &dsp_probe, sizeof (dsp_probe)); 
	// from DSP to PC
	conv_dsp_probe ();

	if (dsp_probe.vector_head < EXTERN_PROBE_VECTOR_HEAD_DEFAULT || index > 40 || index < 0){
		sranger_mk2_hwi_pi.app->message("Error reading Probe Vector:\n Bad vector address, aborting request.\n\n");
		return;
	}

	lseek (dsp, dsp_probe.vector_head + ((index*SIZE_OF_PROBE_VECTOR)<<1), SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_read (dsp, &dsp_vector, SIZE_OF_PROBE_VECTOR<<1);

	// from DSP to PC
        conv_dsp_vector();

	// copy/convert from DSP format
	__dsp_vector->n = dsp_vector.n;
	__dsp_vector->dnx = dsp_vector.dnx;
	__dsp_vector->srcs = dsp_vector.srcs;
	__dsp_vector->options = dsp_vector.options;
	__dsp_vector->ptr_fb = dsp_vector.ptr_fb;
	__dsp_vector->repetitions = dsp_vector.repetitions;
	__dsp_vector->i = dsp_vector.i;
	__dsp_vector->j = dsp_vector.j;
	__dsp_vector->ptr_next = dsp_vector.ptr_next;
	__dsp_vector->ptr_final = dsp_vector.ptr_final;
	__dsp_vector->f_du = dsp_vector.f_du;
	__dsp_vector->f_dx = dsp_vector.f_dx;
	__dsp_vector->f_dy = dsp_vector.f_dy;
	__dsp_vector->f_dz = dsp_vector.f_dz;
	__dsp_vector->f_dx0 = dsp_vector.f_dx0;
	__dsp_vector->f_dy0 = dsp_vector.f_dy0;
	__dsp_vector->f_dphi = dsp_vector.f_dphi;
}


void sranger_mk3_hwi_dev::write_dsp_abort_probe () {
	PROBE_MK3 dsp_probe;
	dsp_probe.start = 0;
	dsp_probe.stop  = 1;
        CONV_32 (dsp_probe.start);
        CONV_32 (dsp_probe.stop);
        g_message ("ABORT PROBE REQUESTED.");
        lseek (dsp, magic_data.probe, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
        sr_write (dsp, &dsp_probe,  (2*2)<<1); 
        usleep ((useconds_t) (5000) ); // give some time to abort
}


int sranger_mk3_hwi_dev::read_pll_symbols (){

	PI_DEBUG_GP (DBG_L4, "sranger_mk3_hwi_dev::read_pll_symbols\n");
	lseek (dsp, magic_data.PLL_lookup, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_read (dsp, &dsp_pll, sizeof (dsp_pll)); 


// **** THESE ARE ONLY ADRESSES FOR ACCESS!!! NOT THE DATA ITSELF ******
	CONV_32 (dsp_pll.pSignal1);
	CONV_32 (dsp_pll.pSignal2);
	CONV_32 (dsp_pll.Signal1);
	CONV_32 (dsp_pll.Signal2);
// Block length
	CONV_32 (dsp_pll.blcklen); // Max MaxSizeSigBuf-1 
	
// Sine variables
	CONV_32 (dsp_pll.deltaReT); 
	CONV_32 (dsp_pll.deltaImT); 
	CONV_32 (dsp_pll.volumeSine); 
 
// Phase/Amp detector time constant 
	CONV_32 (dsp_pll.Tau_PAC);

// Phase corrector variables
	CONV_32 (dsp_pll.PI_Phase_Out); 
	CONV_32 (dsp_pll.pcoef_Phase); 
	CONV_32 (dsp_pll.icoef_Phase); 
	CONV_32 (dsp_pll.errorPI_Phase); 
	CONV_32 (dsp_pll.memI_Phase); 
	CONV_32 (dsp_pll.setpoint_Phase); 
	CONV_32 (dsp_pll.ctrlmode_Phase); 
	CONV_32 (dsp_pll.corrmin_Phase); 
	CONV_32 (dsp_pll.corrmax_Phase); 
	
// Min/Max for the output signals
	CONV_32 (dsp_pll.ClipMin); 
	CONV_32 (dsp_pll.ClipMax); 

// Amp corrector variables
	CONV_32 (dsp_pll.ctrlmode_Amp);
	CONV_32 (dsp_pll.corrmin_Amp); 
	CONV_32 (dsp_pll.corrmax_Amp); 
        CONV_32 (dsp_pll.setpoint_Amp);
	CONV_32 (dsp_pll.pcoef_Amp); 
	CONV_32 (dsp_pll.icoef_Amp); 
	CONV_32 (dsp_pll.errorPI_Amp); 
	CONV_32 (dsp_pll.memI_Amp);
	
// LP filter selections
	CONV_32 (dsp_pll.LPSelect_Phase);
	CONV_32 (dsp_pll.LPSelect_Amp); 

// pointers to live signals
	CONV_32 (dsp_pll.InputFiltered);       // Resonator Output Signal
	CONV_32 (dsp_pll.SineOut0);            // Excitation Signal
	CONV_32 (dsp_pll.phase);               // Resonator Phase (no LP filter)
	CONV_32 (dsp_pll.PI_Phase_OutMonitor); // Excitation Freq. (no LP filter)
	CONV_32 (dsp_pll.amp_estimation);      // Resonator Amp. (no LP filter)
	CONV_32 (dsp_pll.volumeSineMonitor);   // Excitation Amp. (no LP filter) (same as volumeSine)
	CONV_32 (dsp_pll.Filter64Out);         // Filter64Out [4]: Excitation Freq. (with LP filter), ...

	return 0;
} // returns 0 if PLL capability available, else -1

// #define PAC_DEBUGL0_PRINT(format,args...) { SRANGER_DEBUG_GP(format,##args); }
#define PAC_DEBUGL0_PRINT(format,args...) {;}

//#define PAC_DEBUGLR_PRINT(format,args...) { SRANGER_DEBUG_GP(format,##args); }
#define PAC_DEBUGLR_PRINT(format,args...) {;}

#define PAC_DEBUGL1_PRINT(format,args...) { SRANGER_DEBUG_GP(format,##args); }
//#define PAC_DEBUGL1_PRINT(format,args...) {;}

#define PAC_DEBUGL2_PRINT(format,args...) { SRANGER_DEBUG_GP(format,##args); }
//#define PAC_DEBUGL2_PRINT(format,args...) {;}

double sranger_mk3_hwi_dev::read_pll_variable32 (gint64 address){
	gint32 x;
	lseek (dsp, address, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_read (dsp, &x, sizeof (x)); 
	CONV_32 (x);
	PAC_DEBUGL0_PRINT ("sranger_mk3_hwi_dev::read_variable32[0x%06x] = %d\n", address, x);
	return (double)x;
}

gint32 sranger_mk3_hwi_dev::read_pll_variable32i (gint64 address){
	gint32 x;
	lseek (dsp, address, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_read (dsp, &x, sizeof (x)); 
	CONV_32 (x);
	PAC_DEBUGL0_PRINT ("sranger_mk3_hwi_dev::read_variable32[0x%06x] = %d\n", address, x);
	return (double)x;
}

void sranger_mk3_hwi_dev::write_pll_variable32 (gint64 address, double val){
	gint32 x = (gint32)val;
	PAC_DEBUGL0_PRINT ("sranger_mk3_hwi_dev::write_variable32[0x%06x] = %d\n", address, x);
	CONV_32 (x);
	lseek (dsp, address, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_write (dsp, &x, sizeof (x)); 
}

void sranger_mk3_hwi_dev::write_pll_variable32 (gint64 address, gint32 val){
	gint32 x = val;
	PAC_DEBUGL0_PRINT ("sranger_mk3_hwi_dev::write_variable32[0x%06x] = %d\n", address, x);
	CONV_32 (x);
	lseek (dsp, address, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_write (dsp, &x, sizeof (x)); 
}

// 64bit writes must be atomic!!!

double sranger_mk3_hwi_dev::read_pll_variable64 (gint64 address){
	union { gint32 a,b; gint64 x; } v64;
//	if (ioctl(dsp, SRANGER_MK3_IOCTL_NON_ATOMIC_TRANSFER, 0) < 0){
//		PAC_DEBUGL1_PRINT ("sranger_mk3_hwi_dev::write_pll_variable64:  ERROR SWITCHING MK3 TO ATOMIC TRANSFER.\n");
//		return 0.;
//	}
	lseek (dsp, address, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_read (dsp, &v64.x, sizeof (v64.x)); 
//	if ((ioctl(dsp, SRANGER_MK3_IOCTL_ATOMIC_TRANSFER, 0)) < 0){
//		PAC_DEBUGL1_PRINT ("sranger_mk3_hwi_dev::write_pll_variable64:  ERROR SWITCHING MK3 BACK TO NON-ATOMIC TRANSFER.\n");
//	}
	CONV_32 (v64.a);
	CONV_32 (v64.b);
	return (double)v64.x;
}

void sranger_mk3_hwi_dev::write_pll_variable64 (gint64 address, gint64 val){
	union { gint32 a,b; gint64 x; } v64;
	v64.x = val;
	CONV_32 (v64.a);
	CONV_32 (v64.b);	
//	if (ioctl(dsp, SRANGER_MK3_IOCTL_NON_ATOMIC_TRANSFER, 0) < 0){
//		PAC_DEBUGL1_PRINT ("sranger_mk3_hwi_dev::write_pll_variable64:  ERROR SWITCHING MK3 TO ATOMIC TRANSFER.\n");
//		return;
//	}
	lseek (dsp, address, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_write (dsp, &v64.x, sizeof (v64.x)); 
//	if ((ioctl(dsp, SRANGER_MK3_IOCTL_ATOMIC_TRANSFER, 0)) < 0){
//		PAC_DEBUGL1_PRINT ("sranger_mk3_hwi_dev::write_pll_variable64:  ERROR SWITCHING MK3 BACK TO NON-ATOMIC TRANSFER.\n");
//		return;
//	}
}

void sranger_mk3_hwi_dev::write_pll_variable64 (gint64 address, double val){
	write_pll_variable64 (address, (gint64)val);
}

void sranger_mk3_hwi_dev::read_pll_array32 (gint64 address, int n, gint32 *arr){
	lseek (dsp, address, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_read (dsp, arr, n * sizeof (arr[0])); 
	for (int i=0; i<n; ++i) 
		CONV_32 (arr[i]);
}

void sranger_mk3_hwi_dev::write_pll_array32 (gint64 address, int n, gint32 *arr){
	for (int i=0; i<n; ++i) 
		CONV_32 (arr[i]);
	lseek (dsp, address, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_write (dsp, arr, n * sizeof (arr[0])); 
	for (int i=0; i<n; ++i) 
		CONV_32 (arr[i]);
}

int sranger_mk3_hwi_dev::read_pll (PAC_control &pll, PLL_GROUP group){
	// make sure we got tht symbols read
	if (group & PLL_SYMBOLS || dsp_pll.volumeSine == 0)
		read_pll_symbols ();

	if (group & PLL_TAUPAC){
		PAC_DEBUGL1_PRINT ("sranger_mk3_hwi_dev::read_pll -- PLL_TAU_PAC --\n");
		double tau = read_pll_variable32 (dsp_pll.Tau_PAC);
		if (tau > 0.)
			pll.Tau_PAC      = CPN(22)*1.5e-5 / tau;
		else
			pll.Tau_PAC      = 0.;

		PAC_DEBUGL1_PRINT ("sranger_mk3_hwi_dev::read_pll Tau_PAC = %g s\n", pll.Tau_PAC);
	}

	if (group & PLL_SINE){
		PAC_DEBUGL1_PRINT ("sranger_mk3_hwi_dev::read_pll -- PLL_SINE --\n");
		pll.deltaReT     = read_pll_variable32 (dsp_pll.deltaReT);
		pll.deltaImT     = read_pll_variable32 (dsp_pll.deltaImT);

		// pll.FSineHz      = pll.deltaImT*150000./(2.*M_PI);
		// re = CPN(31), im = 2pi Freq / 150000

		pll.FSineHz      = read_pll_variable32 (dsp_pll.PI_Phase_Out)*150000./(CPN(29)*2.*M_PI);
		pll.PI_Phase_Out = pll.FSineHz;
		pll.volumeSine     = read_pll_variable32 (dsp_pll.volumeSine)*10./CPN(22);
		PAC_DEBUGL1_PRINT ("sranger_mk3_hwi_dev::read_pll deltaReT = %g\n", pll.deltaReT);
		PAC_DEBUGL1_PRINT ("sranger_mk3_hwi_dev::read_pll deltaImT = %g\n", pll.deltaImT);
		PAC_DEBUGL1_PRINT ("sranger_mk3_hwi_dev::read_pll PI_Phase_Out = %g Hz (=FSineHz)\n", pll.PI_Phase_Out);
		PAC_DEBUGL1_PRINT ("sranger_mk3_hwi_dev::read_pll volumeSine   = %g V\n", pll.volumeSine);
	
	}

	if (group & PLL_CONTROLLER_PHASE){
		PAC_DEBUGL1_PRINT ("sranger_mk3_hwi_dev::read_pll -- PLL_CONTROLLER_PHASE --\n");
		pll.ctrlmode_Phase = read_pll_variable32 (dsp_pll.ctrlmode_Phase);
		pll.setpoint_Phase = 0; read_pll_variable32 (dsp_pll.setpoint_Phase)*180./(CPN(22)*M_PI);
		pll.corrmax_Phase  = read_pll_variable64 (dsp_pll.corrmax_Phase)*150000./(CPN(58)*2.*M_PI);
		pll.corrmin_Phase  = read_pll_variable64 (dsp_pll.corrmin_Phase)*150000./(CPN(58)*2.*M_PI);
		pll.memI_Phase     = read_pll_variable64 (dsp_pll.memI_Phase)*150000./(CPN(58)*2.*M_PI);
		// pll.icoef_Phase    = read_pll_variable32 (dsp_pll.icoef_Phase);
		// = ISign * CPN(29)*pow(10.,Igain/20.);

		// pll.pcoef_Phase    = read_pll_variable32 (dsp_pll.pcoef_Phase);
		// = PSign * CPN(29)*pow(10.,Pgain/20.);
		
		pll.errorPI_Phase  = read_pll_variable32 (dsp_pll.errorPI_Phase);

		PAC_DEBUGL1_PRINT ("sranger_mk3_hwi_dev::read_pll -- ctrlmode_Phase = %d : %s\n", pll.ctrlmode_Phase, pll.ctrlmode_Phase?"ON":"OFF");
		PAC_DEBUGL1_PRINT ("sranger_mk3_hwi_dev::read_pll -- setpoint_Phase = %g\n", pll.setpoint_Phase);
		PAC_DEBUGL1_PRINT ("sranger_mk3_hwi_dev::read_pll -- corrmin_Phase = %g\n", pll.corrmax_Phase);
		PAC_DEBUGL1_PRINT ("sranger_mk3_hwi_dev::read_pll -- corrmax_Phase = %g\n", pll.corrmin_Phase);
		PAC_DEBUGL1_PRINT ("sranger_mk3_hwi_dev::read_pll -- memI_Phase = %g\n", pll.memI_Phase);
		PAC_DEBUGL1_PRINT ("sranger_mk3_hwi_dev::read_pll -- errorPI_Phase = %g\n", pll.errorPI_Phase);
	}

	if (group & PLL_CONTROLLER_AMPLITUDE){
		PAC_DEBUGL1_PRINT ("sranger_mk3_hwi_dev::read_pll -- PLL_CONTROLLER_AMPLITIDE --\n");
		pll.ctrlmode_Amp = read_pll_variable32 (dsp_pll.ctrlmode_Amp);
		
		pll.setpoint_Amp = read_pll_variable32 (dsp_pll.setpoint_Amp)*10./(CPN(29));
		pll.corrmax_Amp  = read_pll_variable64 (dsp_pll.corrmax_Amp)*10./CPN(58);
		pll.corrmin_Amp  = read_pll_variable64 (dsp_pll.corrmin_Amp)*10./CPN(58);
		pll.memI_Amp     = read_pll_variable64 (dsp_pll.memI_Amp)*10./CPN(58);
		// pll.icoef_Amp    = read_pll_variable32 (dsp_pll.icoef_Amp);
		// = ISign * CPN(29)*pow(10.,Igain/20.);
		
		// pll.pcoef_Amp    = read_pll_variable32 (dsp_pll.pcoef_Amp);
		// = PSign * CPN(29)*pow(10.,Pgain/20.);
		
		pll.errorPI_Amp  = read_pll_variable32 (dsp_pll.errorPI_Amp);

		PAC_DEBUGL1_PRINT ("sranger_mk3_hwi_dev::read_pll -- ctrlmode_Amp = %d : %s\n", pll.ctrlmode_Amp, pll.ctrlmode_Amp?"ON":"OFF");
		PAC_DEBUGL1_PRINT ("sranger_mk3_hwi_dev::read_pll -- setpoint_Amp = %g V\n", pll.setpoint_Amp);
		PAC_DEBUGL1_PRINT ("sranger_mk3_hwi_dev::read_pll -- corrmin_Amp = %g V\n", pll.corrmax_Amp);
		PAC_DEBUGL1_PRINT ("sranger_mk3_hwi_dev::read_pll -- corrmax_Amp = %g V\n", pll.corrmin_Amp);
		PAC_DEBUGL1_PRINT ("sranger_mk3_hwi_dev::read_pll -- memI_Amp = %g\n", pll.memI_Amp);
		PAC_DEBUGL1_PRINT ("sranger_mk3_hwi_dev::read_pll -- errorPI_Amp = %g V\n", pll.errorPI_Amp);
	}

	if (group & PLL_OPERATION){
		PAC_DEBUGL1_PRINT ("sranger_mk3_hwi_dev::read_pll -- PLL_OPERATION --\n");
		pll.LPSelect_Phase = read_pll_variable32 (dsp_pll.LPSelect_Phase);
		pll.LPSelect_Amp   = read_pll_variable32 (dsp_pll.LPSelect_Amp);

		PAC_DEBUGL1_PRINT ("sranger_mk3_hwi_dev::read_pll -- LPSelect_Phase = %d\n", pll.LPSelect_Phase);
		PAC_DEBUGL1_PRINT ("sranger_mk3_hwi_dev::read_pll -- LPSelect_Amp   = %d\n", pll.LPSelect_Amp);

		// 0: Excitation Frequency
		// 1: Resonator Phase
		// 2: Excitation Amplitude
		// 3: Resonator Amplitude
		double factor2dsp[4] = { CPN(29)*2.*M_PI/150000.,
					 CPN(29)*M_PI/180., 
					 CPN(29)/10., 
					 CPN(29)/10. 
		};
		
		gint32 x[4];
		read_pll_array32 (dsp_pll.ClipMin, 4, x);
		// 2 world
		for (int i=0; i<4; ++i){
			pll.ClipMin[i] = ((double)x[i]-1.)/factor2dsp[i];
			PAC_DEBUGL1_PRINT ("sranger_mk3_hwi_dev::read_pll -- ClipMin[%d] = %g\n", i, pll.ClipMin[i]);
		}
		// 2 DSP ==>  DSPPLL.ClipMin[i] = factor2dsp[i] * x + 1.;
		
		read_pll_array32 (dsp_pll.ClipMax, 4, x);
		for (int i=0; i<4; ++i){
			pll.ClipMax[i] =  ((double)x[i]+1.)/factor2dsp[i];
			PAC_DEBUGL1_PRINT ("sranger_mk3_hwi_dev::read_pll -- ClipMax[%d] = %g\n", i, pll.ClipMax[i]);
		}
	}

	if (group & PLL_READINGS){
		PAC_DEBUGLR_PRINT ("sranger_mk3_hwi_dev::read_pll -- PLL_READINGS --\n");
		gint32 F64[4];
		double factor[4] = { 150000./(CPN(29)*2.*M_PI), // Excit. Freq
				     180./(CPN(29)*M_PI), // ResPhase
				     10./CPN(29), // ResAmp LP
				     10./CPN(29)  // ExecAmp LP
		};

		read_pll_array32 (dsp_pll.Filter64Out, 4, F64);
		for (int i=0; i<4; ++i)
			pll.Filter64Out[i] = (double)F64[i] * factor[i];

		PAC_DEBUGLR_PRINT ("sranger_mk3_hwi_dev::read_pll -- Filter64Out[0] excitation = %g Hz\n",  pll.Filter64Out[F64_Excitation]);
		PAC_DEBUGLR_PRINT ("sranger_mk3_hwi_dev::read_pll -- Filter64Out[1] ResPhaseLP = %g deg\n", pll.Filter64Out[F64_ResPhaseLP]);
		PAC_DEBUGLR_PRINT ("sranger_mk3_hwi_dev::read_pll -- Filter64Out[2] ResAmpLP   = %g V\n",   pll.Filter64Out[F64_ResAmpLP]);
		PAC_DEBUGLR_PRINT ("sranger_mk3_hwi_dev::read_pll -- Filter64Out[3] ExecAmpLP  = %g V\n",   pll.Filter64Out[F64_ExecAmpLP]);

		// life Monitoring
		pll.phase          = read_pll_variable32 (dsp_pll.phase) * 180./(CPN(22)*M_PI);
		pll.amp_estimation = read_pll_variable32 (dsp_pll.amp_estimation) * 10./CPN(22);
		pll.res_gain_computed = pll.Filter64Out[F64_ResAmpLP]/pll.Filter64Out[F64_ExecAmpLP];
//		pll.InputFiltered  = read_pll_variable32 (dsp_pll.InputFiltered);
//		pll.PI_Phase_Out   = read_pll_variable32 (dsp_pll.PI_Phase_Out);
//		pll.volumeSine     = read_pll_variable32 (dsp_pll.volumeSine);
//		pll.SineOut0       = read_pll_variable32 (dsp_pll.SineOut0);
	}

//	pll.blcklen = read_pll_variable32 (dsp_pll.blcklen);
//	read_pll_arrya32 (dsp_pll.Signal1, pll.blcklen, pll.Signal1);
//	read_pll_arrya32 (dsp_pll.Signal2, pll.blcklen, pll.Signal2);

	return 0;
} // returns 0 if PLL capability available, else -1

void sranger_mk3_hwi_dev::set_blcklen (int len){
	write_pll_variable32 (dsp_pll.blcklen, (gint32)len);
}


#define MK__i32_off(n) ((n)<<2)

// needs set_blcklen to finish and write!
void sranger_mk3_hwi_dev::set_scope (int s1, int s2){
	if (s1 >= 0)
	    switch (s1){
	    case 0: write_pll_variable32 (dsp_pll.pSignal1, (gint32)dsp_pll.Filter64Out+0); break;
	    case 1: write_pll_variable32 (dsp_pll.pSignal1, (gint32)dsp_pll.Filter64Out+4); break;
	    case 2: write_pll_variable32 (dsp_pll.pSignal1, (gint32)dsp_pll.Filter64Out+8); break;
	    case 3: write_pll_variable32 (dsp_pll.pSignal1, (gint32)dsp_pll.Filter64Out+12); break;
	    case 4: write_pll_variable32 (dsp_pll.pSignal1, (gint32)dsp_pll.amp_estimation); break;
	    case 5: write_pll_variable32 (dsp_pll.pSignal1, (gint32)dsp_pll.phase); break;
	    case 6: write_pll_variable32 (dsp_pll.pSignal1, (gint32)dsp_pll.SineOut0); break;
	    case 7: write_pll_variable32 (dsp_pll.pSignal1, (gint32)dsp_pll.InputFiltered); break;
//	    case 8: write_pll_variable32 (dsp_pll.pSignal1, (gint32)(magic_data.feedback + MK__i32_off (4))); break; // cb_Ic
//	    case 9: write_pll_variable32 (dsp_pll.pSignal1, (gint32)(magic_data.feedback + MK__i32_off (19))); break; // z32neg
//	    case 10: write_pll_variable32 (dsp_pll.pSignal1, (gint32)(magic_data.feedback + MK__i32_off (12))); break; // I_iir
//	    case 11: write_pll_variable32 (dsp_pll.pSignal1, (gint32)(magic_data.feedback + MK__i32_off (17))); break; // I_avg
//	    case 12: write_pll_variable32 (dsp_pll.pSignal1, (gint32)(magic_data.feedback + MK__i32_off (18))); break; // I_rms
//	    case 13: write_pll_variable32 (dsp_pll.pSignal1, (gint32)(magic_data.feedback_mixer + MK__i32_off (26))); break; // delta
	    case 14: write_pll_variable32 (dsp_pll.pSignal1, (gint32)(magic_data.scan + MK__i32_off (30))); break; // xyz_vec[0]
	    case 15: write_pll_variable32 (dsp_pll.pSignal1, (gint32)(magic_data.scan + MK__i32_off (31))); break; // xyz_vec[1]
	    case 16: write_pll_variable32 (dsp_pll.pSignal1, (gint32)(magic_data.scan + MK__i32_off (32))); break; // xyz_vec[2]
	    case 17: write_pll_variable32 (dsp_pll.pSignal1, (gint32)(magic_data.scan + MK__i32_off (33))); break; // xy_r_vec[0]
	    case 18: write_pll_variable32 (dsp_pll.pSignal1, (gint32)(magic_data.scan + MK__i32_off (34))); break; // xy_r_vec[1]
	    case 19: write_pll_variable32 (dsp_pll.pSignal1, (gint32)(magic_data.AIC_in + MK__i32_off (0))); break; // IN_0
	    case 20: write_pll_variable32 (dsp_pll.pSignal1, (gint32)(magic_data.AIC_out + MK__i32_off (0))); break; // OUT_0
	    }
	else
	    write_pll_variable32 (dsp_pll.pSignal1, (gint32)(-s1));

	if (s2 >= 0)
	    switch (s2){
	    case 0: write_pll_variable32 (dsp_pll.pSignal2, (gint32)dsp_pll.Filter64Out+0); break;
	    case 1: write_pll_variable32 (dsp_pll.pSignal2, (gint32)dsp_pll.Filter64Out+4); break;
	    case 2: write_pll_variable32 (dsp_pll.pSignal2, (gint32)dsp_pll.Filter64Out+8); break;
	    case 3: write_pll_variable32 (dsp_pll.pSignal2, (gint32)dsp_pll.Filter64Out+12); break;
	    case 4: write_pll_variable32 (dsp_pll.pSignal2, (gint32)dsp_pll.amp_estimation); break;
	    case 5: write_pll_variable32 (dsp_pll.pSignal2, (gint32)dsp_pll.phase); break;
	    case 6: write_pll_variable32 (dsp_pll.pSignal2, (gint32)dsp_pll.SineOut0); break;
	    case 7: write_pll_variable32 (dsp_pll.pSignal2, (gint32)dsp_pll.InputFiltered); break;
//	    case 8: write_pll_variable32 (dsp_pll.pSignal2, (gint32)(magic_data.feedback + MK__i32_off (4))); break; // cb_Ic
//	    case 9: write_pll_variable32 (dsp_pll.pSignal2, (gint32)(magic_data.feedback + MK__i32_off (19))); break; // z32neg
//	    case 10: write_pll_variable32 (dsp_pll.pSignal2, (gint32)(magic_data.feedback + MK__i32_off (12))); break; // I_iir
//	    case 11: write_pll_variable32 (dsp_pll.pSignal2, (gint32)(magic_data.feedback + MK__i32_off (17))); break; // I_avg
//	    case 12: write_pll_variable32 (dsp_pll.pSignal2, (gint32)(magic_data.feedback + MK__i32_off (18))); break; // I_rms
//	    case 13: write_pll_variable32 (dsp_pll.pSignal2, (gint32)(magic_data.feedback_mixer + MK__i32_off (26))); break; // delta
	    case 14: write_pll_variable32 (dsp_pll.pSignal2, (gint32)(magic_data.scan + MK__i32_off (30))); break; // xyz_vec[0]
	    case 15: write_pll_variable32 (dsp_pll.pSignal2, (gint32)(magic_data.scan + MK__i32_off (31))); break; // xyz_vec[1]
	    case 16: write_pll_variable32 (dsp_pll.pSignal2, (gint32)(magic_data.scan + MK__i32_off (32))); break; // xyz_vec[2]
	    case 17: write_pll_variable32 (dsp_pll.pSignal2, (gint32)(magic_data.scan + MK__i32_off (33))); break; // xy_r_vec[0]
	    case 18: write_pll_variable32 (dsp_pll.pSignal2, (gint32)(magic_data.scan + MK__i32_off (34))); break; // xy_r_vec[1]
	    case 19: write_pll_variable32 (dsp_pll.pSignal2, (gint32)(magic_data.AIC_in + MK__i32_off (0))); break; // IN_0
	    case 20: write_pll_variable32 (dsp_pll.pSignal2, (gint32)(magic_data.AIC_out + MK__i32_off (0))); break; // OUT_0
	    }
	else
	    write_pll_variable32 (dsp_pll.pSignal2, (gint32)(-s2));
}


void sranger_mk3_hwi_dev::setup_step_response (double dPhase, double dAmplitude){
	PAC_DEBUGL1_PRINT ("SR-MK3-HwI-PAC:: setup_step_response %g Deg, %g V\n", dPhase, dAmplitude);

	double re=0.;
	gint32 start_call[2]={0,0}; // => exec ChangeFreqHL () on DSP after adjused Re/Im.

	if (dPhase > 0.)
	    re = CPN(29) * dPhase * M_PI / 180.;
	if (dAmplitude > 0.)
	    re = CPN(29) * dAmplitude / 10.;
			
	PAC_DEBUGL1_PRINT ("SR-MK3-HwI-PAC:: write_pll -- deltaReT= %d\n", (int)re);
	write_pll_variable32 (dsp_pll.deltaReT, re);
			
	lseek (dsp, magic_data.PLL_lookup, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	if (dAmplitude > 0.){
	    start_call[0]=4;
	    PAC_DEBUGL1_PRINT ("SR-MK3-HwI-PAC:: write_pll ==> exec TestAmpPIRest_HL()\n");
	} else {
	    start_call[0]=3;
	    PAC_DEBUGL1_PRINT ("SR-MK3-HwI-PAC:: write_pll ==> exec TestPhasePIResp_HL()\n");
	} 

	CONV_32 (start_call[0]);
	CONV_32 (start_call[1]);
	sr_write (dsp, &start_call, 2*sizeof (gint32)); 

//			if (ioctl(dsp, SRANGER_MK2_IOCTL_KERNEL_EXEC, Address of ChangeFreqHL  ) < 0){
//				PAC_DEBUGL1_PRINT ("sranger_mk3_hwi_dev::write_pll:  ERROR K_EXEC call to ChangeFreqFB()\n");
//			}
}

int sranger_mk3_hwi_dev::read_pll_signal1 (gfloat *signal, int n, double scale, gint flag){
	gint blcklen = read_pll_variable32i (dsp_pll.blcklen);

	if (blcklen == -1){
                if (n > 0 && n <= PAC_BLCKLEN){
                        gint32 tmp_array[PAC_BLCKLEN];
                        read_pll_array32 (dsp_pll.Signal1, n, tmp_array);
                        PAC_DEBUGL0_PRINT ("adr  Signal1=%x\n", dsp_pll.Signal1);
                        PAC_DEBUGL0_PRINT ("adr pSignal1=%x\n", dsp_pll.pSignal1);
                        PAC_DEBUGL0_PRINT ("--- pSignal1=%x\n", (gint32)read_pll_variable32 (dsp_pll.pSignal1));
                        for (int i=0; i<n; ++i)
                                signal[i] = (gfloat)tmp_array[i] * scale;
                }
                if (flag)
                        set_blcklen (n);
                return 0;
	}
	return -1;
}
    
int sranger_mk3_hwi_dev::read_pll_signal2 (gfloat *signal, int n, double scale, gint flag){
	gint blcklen = read_pll_variable32i (dsp_pll.blcklen);
	
	if (blcklen == -1){
                if (n > 0 && n <= PAC_BLCKLEN){
                        gint32 tmp_array[PAC_BLCKLEN];
                        read_pll_array32 (dsp_pll.Signal2, n, tmp_array);
                        PAC_DEBUGL0_PRINT ("adr  Signal2=%x\n", dsp_pll.Signal2);
                        PAC_DEBUGL0_PRINT ("adr pSignal2=%x\n", dsp_pll.pSignal2);
                        PAC_DEBUGL0_PRINT ("--- pSignal2=%x\n", (gint32)read_pll_variable32 (dsp_pll.pSignal2));
                        for (int i=0; i<n; ++i)
                                signal[i] = (gfloat)tmp_array[i] * scale;
                }
                if (flag)
                        set_blcklen (n);
                return 0;
	}
	return -1;
}

int sranger_mk3_hwi_dev::read_pll_signal1dec (gfloat *signal, int n, double scale, gint record){
        const gint64 max_age = 60000000; // 60s
        static gint64 time_of_last_stamp = 0; // abs time in us
        static gint32 ring_buffer_position_last = -1;
	gint32 deci;
        read_pll_array32 (dsp_pll.Signal1+4*0x7ffff, 1, &deci);
        //g_print ("deci=%d %08x\n",deci, deci);
        if (n > 0 && n <= 0x40000){
                gint32 tmp_array[0x40000];
                gint32 start = deci-n;
                if (start > 0)
                        read_pll_array32 (dsp_pll.Signal1+((0x80000+start)<<2), n, tmp_array);
                else{
                        gint n1 = -start;
                        gint n2 = n-n1;
                        read_pll_array32 (dsp_pll.Signal1+((0x80000+0x40000-n1)<<2), n1, tmp_array);
                        read_pll_array32 (dsp_pll.Signal1+((0x80000)<<2), n2, tmp_array+n1);
                }
                for (int i=0; i<n; ++i){
                        signal[i] = (gfloat)tmp_array[i] * scale;
                        //g_print ("%d %d\n", i,tmp_array[i]);
                }
                if (record && ring_buffer_position_last>=0){
                        std::ofstream f;
                        f.open("gxsm4-recorder-logfile-signal1-dec256", std::ios::out | std::ios::app);
                        if(f.good()){
                                // append new data from ring_buffer_position_last ... deci (now)
                                int k=0;
                                if ((time_of_last_stamp+max_age) < g_get_real_time () ){
                                        time_of_last_stamp = g_get_real_time ();
                                
                                        f << "## " <<  time_of_last_stamp << std::endl;
                                }
                                
                                f << "# " << ring_buffer_position_last << " ... " << deci << std::endl;
                                if (ring_buffer_position_last < deci)
                                        for (int i=n-(deci-ring_buffer_position_last); i<n; ++i)
                                                //f << (ring_buffer_position_last+k++) << " " << tmp_array[i] << std::endl;
                                                f << tmp_array[i] << std::endl;
                                else
                                        for (int i=n-(deci+0x40000-ring_buffer_position_last); i<n; ++i)
                                                //f << (ring_buffer_position_last+k++) << " " << tmp_array[i] << " L" << std::endl;
                                                f << tmp_array[i] << " L" << std::endl;
                                f.close ();
                        }
                }
        }
        ring_buffer_position_last = deci;
	return -1;
}

int sranger_mk3_hwi_dev::read_pll_signal2dec (gfloat *signal, int n, double scale, gint record){
        static gint32 ring_buffer_position_last = -1;
	gint32 deci;
        read_pll_array32 (dsp_pll.Signal1+4*0x7ffff, 1, &deci);
        if (n > 0 && n <= 0x40000){
                gint32 tmp_array[0x40000];
                gint32 start = deci-n;
                if (start > 0)
                        read_pll_array32 (dsp_pll.Signal2+((0x80000+start)<<2), n, tmp_array);
                else{
                        gint n1 = -start;
                        gint n2 = n-n1;
                        read_pll_array32 (dsp_pll.Signal2+((0x80000+0x40000-n1)<<2), n1, tmp_array);
                        read_pll_array32 (dsp_pll.Signal2+((0x80000+n2)<<2), n2, tmp_array+n1);
                }
                for (int i=0; i<n; ++i)
                        signal[i] = (gfloat)tmp_array[i] * scale;

                if (record && ring_buffer_position_last>=0){
                        std::ofstream f;
                        f.open("gxsm4-recorder-logfile-signal2-dec256", std::ios::out | std::ios::app);
                        if(f.good()){
                                // append new data from ring_buffer_position_last ... deci (now)
                                int k=0;
                                f << "# " << ring_buffer_position_last << " ... " << deci << std::endl;
                                if (ring_buffer_position_last < deci)
                                        for (int i=n-(deci-ring_buffer_position_last); i<n; ++i)
                                                //f << (ring_buffer_position_last+k++) << " "  << tmp_array[i] << std::endl;
                                                f << tmp_array[i] << std::endl;
                                else
                                        for (int i=n-(deci+0x40000-ring_buffer_position_last); i<n; ++i)
                                                //f << (ring_buffer_position_last+k++) << " "  << tmp_array[i] << " L" << std::endl;
                                                f << tmp_array[i] << std::endl;
                                f.close ();
                        }
                }
        }
        ring_buffer_position_last = deci;
	return -1;
}



int sranger_mk3_hwi_dev::write_pll (PAC_control &pll, PLL_GROUP group, int enable){
	static int wait=0;
	static int pll_old_init=1;
	static PAC_control pll_old;
	if (pll_old_init){
		read_pll (pll_old, PLL_ALL);
		pll_old_init=0;
	}


	if (group & PLL_SYMBOLS || dsp_pll.volumeSine == 0){
		PAC_DEBUGL1_PRINT ("SR-MK3-HwI-PAC:: write_pll -- read symbols\n");
		read_pll_symbols ();
	}

	if (group & PLL_TAUPAC){
		PAC_DEBUGL1_PRINT ("SR-MK3-HwI-PAC:: write_pll -- PLL_TAUPAC\n");
		if (pll.Tau_PAC != pll_old.Tau_PAC){
			if (pll.Tau_PAC > 0.){
				PAC_DEBUGL1_PRINT ("SR-MK3-HwI-PAC:: write_pll -- TAU=%g s  >>  %d\n", pll.Tau_PAC, (int)(CPN(22)*1.5e-5/pll.Tau_PAC));
				write_pll_variable32 (dsp_pll.Tau_PAC, (CPN(22)*1.5e-5)/pll.Tau_PAC); // Tau[s]
			} else { // OFF
				PAC_DEBUGL1_PRINT ("SR-MK3-HwI-PAC:: write_pll -- TAU=0000\n");
				write_pll_variable32 (dsp_pll.Tau_PAC, 0.); // Tau[s]
			}
			pll_old.Tau_PAC = pll.Tau_PAC;
		} else { PAC_DEBUGL1_PRINT ("-- no change Tau_PAC.\n"); }
	}

	if (group & PLL_SINE){
		PAC_DEBUGL1_PRINT ("SR-MK3-HwI-PAC:: write_pll -- PLL_SINE\n");
		PAC_DEBUGL1_PRINT ("SR-MK3-HwI-PAC:: write_pll -- VolumeSine=%g V  >>  %d\n",pll.volumeSine,   (int)(CPN(22)*pll.volumeSine/10.));
		write_pll_variable32 (dsp_pll.volumeSine, CPN(22)*pll.volumeSine/10.);

		if (pll.FSineHz != pll_old.FSineHz && !pll.ctrlmode_Phase){
			PAC_DEBUGL1_PRINT ("SR-MK3-HwI-PAC:: write_pll -- Sine Frequency=%g Hz\n", pll.FSineHz);
			double re = round (CPN(31) * cos (2.*M_PI*pll.FSineHz/150000.));
			double im = round (CPN(31) * sin (2.*M_PI*pll.FSineHz/150000.));
			
			PAC_DEBUGL1_PRINT ("SR-MK3-HwI-PAC:: write_pll -- deltaReT= %.0f\n", re);
			write_pll_variable32 (dsp_pll.deltaReT, re);
			PAC_DEBUGL1_PRINT ("SR-MK3-HwI-PAC:: write_pll -- deltaImT= %.0f\n", im);
			write_pll_variable32 (dsp_pll.deltaImT, im);
			// re = CPN(31), im = 2pi Freq / 150000
			
			PAC_DEBUGL1_PRINT ("SR-MK3-HwI-PAC:: write_pll -- Sine PI_Phase_Out=%g deg  >>  %d\n",pll.PI_Phase_Out, (int)(CPN(29)*2.*M_PI*pll.FSineHz/150000.));
			write_pll_variable32 (dsp_pll.PI_Phase_Out, round (CPN(29)*2.*M_PI*pll.FSineHz/150000.));

			pll_old.FSineHz = pll.FSineHz;

			PAC_DEBUGL1_PRINT ("SR-MK3-HwI-PAC:: write_pll ==> Exec ChangeFreqHL()\n");
			lseek (dsp, magic_data.PLL_lookup, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
			gint32 start_call[2]={1,0}; // => exec ChangeFreqHL () on DSP after adjused Re/Im.
			CONV_32 (start_call[0]);
			CONV_32 (start_call[1]);
			sr_write (dsp, &start_call, 2*sizeof (gint32)); 

//			if (ioctl(dsp, SRANGER_MK2_IOCTL_KERNEL_EXEC, Address of ChangeFreqHL  ) < 0){
//				PAC_DEBUGL1_PRINT ("sranger_mk3_hwi_dev::write_pll:  ERROR K_EXEC call to ChangeFreqFB()\n");
//			}
		} else { PAC_DEBUGL1_PRINT ("-- no change FSineHz or Phase Controller engaged.\n"); }
	}

	if (group & PLL_CONTROLLER_PHASE){
		if (wait) return -1;
		wait=1;
		PAC_DEBUGL1_PRINT ("SR-MK3-HwI-PAC:: write_pll -- PLL_CONTROLLER_PHASE\n");
		// turn on or off
		if (pll.ctrlmode_Phase == 1 && pll_old.ctrlmode_Phase == 0){
			PAC_DEBUGL1_PRINT ("SR-MK3-HwI-PAC:: TURN ON -- write_pll PHASE FB: %d ---> ON\n", pll.ctrlmode_Phase);
			write_pll_variable32 (dsp_pll.errorPI_Phase, 0.);
			write_pll_variable64 (dsp_pll.corrmax_Phase, CPN(58)*2.*M_PI*pll.ClipMax[0]/150000.);
			write_pll_variable64 (dsp_pll.corrmin_Phase, CPN(58)*2.*M_PI*pll.ClipMin[0]/150000.);
			write_pll_variable64 (dsp_pll.memI_Phase, CPN(58)*2.*M_PI*pll.FSineHz/150000.);
			write_pll_variable32 (dsp_pll.ctrlmode_Phase, pll.ctrlmode_Phase);
			pll_old.ctrlmode_Phase = 1;
		} else 	if (pll.ctrlmode_Phase == 0 && pll_old.ctrlmode_Phase == 1){
			PAC_DEBUGL1_PRINT ("SR-MK3-HwI-PAC:: TURN OFF -- write_pll PHASE FB: %d ---> OFF\n", pll.ctrlmode_Phase);
			pll_old.ctrlmode_Phase = 0;
			write_pll_variable32 (dsp_pll.ctrlmode_Phase, pll.ctrlmode_Phase);
		}
		write_pll_variable32 (dsp_pll.setpoint_Phase, CPN(22)*M_PI*pll.setpoint_Phase/180.);

		if (group & PLL_CONTROLLER_PHASE_AUTO){
			if (pll_old.auto_set_BW_Phase == pll.auto_set_BW_Phase) { wait=0; return -1; }
			pll_old.auto_set_BW_Phase = pll.auto_set_BW_Phase;
			double cp = 20. * log10 (1.6575e-5  * pll.auto_set_BW_Phase);
			double ci = 20. * log10 (1.7357e-10 * pll.auto_set_BW_Phase*pll.auto_set_BW_Phase);

                        gchar *msg = g_strdup_printf ("Auto Set Phase: CP=%g dB, CI=%g dB", cp, ci);
                        if (main_get_gapp()->question_yes_no (msg)){
				pll.cp_gain_Phase = cp;
				pll.ci_gain_Phase = ci;
			}
                        g_free (msg);
			PAC_DEBUGL1_PRINT ("SR-MK3-HwI-PAC:: write_pll -- PLL_CONTROLLER_PHASE AUTO SET...\n");
		}
		write_pll_variable32 (dsp_pll.icoef_Phase, pll.signum_ci_Phase * CPN(29)*pow (10.,pll.ci_gain_Phase/20.));
		// = ISign * CPN(29)*pow(10.,Igain/20.);
		
		write_pll_variable32 (dsp_pll.pcoef_Phase, pll.signum_cp_Phase * CPN(29)*pow (10.,pll.cp_gain_Phase/20.));
		// = PSign * CPN(29)*pow(10.,Pgain/20.);

		PAC_DEBUGL1_PRINT ("SR-MK3-HwI-PAC:: write_pll -- PLL_CONTROLLER_PHASE SET: Set=%g deg  CP=%g dB, CI=%g dB\n", 
			 pll.setpoint_Phase, pll.cp_gain_Phase, pll.ci_gain_Phase);
		wait=0;
	}

/*
PAC at 15us !!!

Kp = 20*log10(1.6575e-5*Fc)
Ki = 20*log10(1.7357e-10*Fc^2)

Where Fc is the desired bandwidth of the controller in Hz (the suggested range is between 1.5 Hz
to 4.5kHz).
These gains can be used directly in the P and I formulas (values Pgain and Igain). Note that the
maximum value for both gains is +12 dB. The gain must be clipped at +12dB if the maximum
value is reached. Most of the time the P gain reaches the maximum before the I gain. In this
case, the reduction applied on the P gain must be applied on the I gain to assure a stable
behavior of the controller.
*/

	if (group & PLL_CONTROLLER_AMPLITUDE){
		if (wait) return -1;
		wait=1;
		PAC_DEBUGL1_PRINT ("SR-MK3-HwI-PAC:: write_pll -- PLL_CONTROLLER_AMPLITIDE\n");
		// turn on or off
		if (pll.ctrlmode_Amp == 1 && pll_old.ctrlmode_Amp == 0){
			PAC_DEBUGL1_PRINT ("SR-MK3-HwI-PAC:: TURN ON -- write_pll AMPLITUDE FB: %d\n", pll.ctrlmode_Amp);
			write_pll_variable32 (dsp_pll.errorPI_Amp, 0.);
			write_pll_variable64 (dsp_pll.corrmax_Amp, CPN(58)*pll.ClipMax[2]/10.);
			write_pll_variable64 (dsp_pll.corrmin_Amp, CPN(58)*pll.ClipMin[2]/10.);
			write_pll_variable64 (dsp_pll.memI_Amp, CPN(58)*pll.setpoint_Amp/10);
			write_pll_variable32 (dsp_pll.ctrlmode_Amp, pll.ctrlmode_Amp);
			pll_old.ctrlmode_Amp = 1;
		} else if (pll.ctrlmode_Amp == 0 && pll_old.ctrlmode_Amp == 1){
			PAC_DEBUGL1_PRINT ("SR-MK3-HwI-PAC:: TURN OFF -- write_pll AMPLITUDE FB: %d\n", pll.ctrlmode_Amp);
			pll_old.ctrlmode_Amp = 0;
			write_pll_variable32 (dsp_pll.ctrlmode_Amp, pll.ctrlmode_Amp);

                        // reset (20170814) sugget by Alex
                        write_pll_variable32 (dsp_pll.amp_estimation, 0);

		}
		write_pll_variable32 (dsp_pll.setpoint_Amp, CPN(29)*pll.setpoint_Amp/10.);

		if (group & PLL_CONTROLLER_AMPLITUDE_AUTO){
			if (pll_old.auto_set_BW_Amp == pll.auto_set_BW_Amp && 
			    pll_old.Qfactor == pll.Qfactor && 
			    pll_old.GainRes == pll.GainRes ) { wait=0; return -1; }
			pll_old.auto_set_BW_Amp = pll.auto_set_BW_Amp;
			pll_old.Qfactor = pll.Qfactor;
			pll_old.GainRes = pll.GainRes;
			double Q = pll.Qfactor;     // Q-factor
			double F0 = pll.FSineHz; // Res. Freq
			double Fc = pll.auto_set_BW_Amp; // 1.5 .. 10Hz
			double gainres = pll.GainRes;
			double cp = 20. * log10 (0.08045   * Q*Fc / (gainres*F0));
			double ci = 20. * log10 (8.4243e-7 * Q*Fc*Fc / (gainres*F0));
                        gchar *msg = g_strdup_printf ("Auto Set Amp: CP=%g dB, CI=%g dB", cp, ci);
                        if (main_get_gapp()->question_yes_no (msg)){
				pll.cp_gain_Amp = cp;
				pll.ci_gain_Amp = ci;
			}
                        g_free (msg);
			PAC_DEBUGL1_PRINT ("SR-MK3-HwI-PAC:: write_pll -- PLL_CONTROLLER_AMPLITUDE AUTO SET...\n");
		}

		write_pll_variable32 (dsp_pll.icoef_Amp, pll.signum_ci_Amp * CPN(29)*pow (10.,pll.ci_gain_Amp/20.));
		// = ISign * CPN(29)*pow(10.,Igain/20.);
		
		write_pll_variable32 (dsp_pll.pcoef_Amp, pll.signum_cp_Amp * CPN(29)*pow (10.,pll.cp_gain_Amp/20.));
		// = PSign * CPN(29)*pow(10.,Pgain/20.);
		PAC_DEBUGL1_PRINT ("SR-MK3-HwI-PAC:: write_pll -- PLL_CONTROLLER_AMPLITUDE AUTO SET: Set=%g V CP=%g dB, CI=%g dB\n",
			 pll.setpoint_Amp, pll.cp_gain_Amp, pll.ci_gain_Amp);
		wait=0;
	}
/*
Kp = 20*log10(0.08045*QFc/(Gain_res*F0))
Ki = 20*log10(8.4243e-7*QFc^2/(Gain_res*F0))
Where :
Gainres is the gain of the resonator at the resonance
Q is the Q factor of the resonator
F0 is the frequency at the resonance in Hz
Fc is the desired bandwidth of the controller in Hz (the suggested range is between 1.5 Hz to
10Hz).
These gains can be used directly in the P and I formulas (values PgainAmp and IgainAamp). Note that
the maximum value for both gains is +12 dB. The gain must be clipped at +12dB if the maximum
value is reached. Most of the time the P gain reaches the maximum before the I gain. In this
case, the reduction applied on the P gain must be applied on the I gain to assure a stable
behavior of the controller.
*/

	if (group & PLL_OPERATION){
		PAC_DEBUGL1_PRINT ("SR-MK3-HwI-PAC:: write_pll -- PLL_OPERATION\n");

		// 0: Excitation Frequency
		// 1: Resonator Phase
		// 2: Excitation Amplitude (according to doc) [3] !! ???
		// 3: Resonator Amplitude (according to doc) [2]
		double factor2dsp[4] = { CPN(29)*2.*M_PI/150000.,
					 CPN(29)*M_PI/180., 
					 CPN(29)/10., 
					 CPN(29)/10. 
		};
		
		gint32 x[4];
		// 2 world
		for (int i=0; i<4; ++i){
			x[i] = (gint32)(pll.ClipMin[i]*factor2dsp[i]+1.);
			PAC_DEBUGL1_PRINT ("SR-MK3-HwI-PAC:: write_pll -- ClipMin[%d] = %d (%g)\n", i, x[i], pll.ClipMin[i]);
		}
		write_pll_array32 (dsp_pll.ClipMin, 4, x);
		
		// 2 DSP ==>  DSPPLL.ClipMin[i] = factor2dsp[i] * x + 1.;
		
		for (int i=0; i<4; ++i){
			x[i] = (gint32)(pll.ClipMax[i]*factor2dsp[i]+1.);
			PAC_DEBUGL1_PRINT ("SR-MK3-HwI-PAC:: write_pll -- ClipMax[%d] = %d (%g)\n", i, x[i], pll.ClipMax[i]);
		}
		write_pll_array32 (dsp_pll.ClipMax, 4, x);

		write_pll_variable32 (dsp_pll.LPSelect_Phase, pll.LPSelect_Phase);
		write_pll_variable32 (dsp_pll.LPSelect_Amp, pll.LPSelect_Amp);
		PAC_DEBUGL1_PRINT ("sranger_mk3_hwi_dev::write_pll -- LPSelect_Phase = %d\n", pll.LPSelect_Phase);
		PAC_DEBUGL1_PRINT ("sranger_mk3_hwi_dev::write_pll -- LPSelect_Amp   = %d\n", pll.LPSelect_Amp);

		PAC_DEBUGL1_PRINT ("SR-MK3-HwI-PAC:: write_pll ==> OutputSignalSetUp_HL()\n");
		lseek (dsp, magic_data.PLL_lookup, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
		gint32 start_call[2]={2,0}; // => exec ChangeFreqHL () on DSP after adjused Re/Im.
		CONV_32 (start_call[0]);
		CONV_32 (start_call[1]);
		sr_write (dsp, &start_call, 2*sizeof (gint32)); 
	}
		
//	write_pll_variable32 (dsp_pll.blcklen, pll.blcklen);
//	read_pll_arrya32 (dsp_pll.Signal1, pll.blcklen, pll.Signal1);
//	read_pll_arrya32 (dsp_pll.Signal2, pll.blcklen, pll.Signal2);

	return 0;
}

/**
 *  SIGNAL MANAGEMENT
 **/

int sranger_mk3_hwi_dev::lookup_signal_by_ptr(gint64 sigptr){
	for (int i=0; i<NUM_SIGNALS; ++i){
		if (dsp_signal_lookup_managed[i].dim == 1 && sigptr == dsp_signal_lookup_managed[i].p)
			return i;
		gint64 offset = sigptr - (gint64)dsp_signal_lookup_managed[i].p;
		if (sigptr >= dsp_signal_lookup_managed[i].p && offset < 4*dsp_signal_lookup_managed[i].dim){
			dsp_signal_lookup_managed[i].index = offset/4;
			return i;
		}
	}
	return -1;
}

int sranger_mk3_hwi_dev::lookup_signal_by_name(const gchar *sig_name){
	for (int i=0; i<NUM_SIGNALS; ++i)
		if (!strcmp (sig_name, dsp_signal_lookup_managed[i].label))
			return i;
	return -1;
}

const gchar *sranger_mk3_hwi_dev::lookup_signal_name_by_index(int i){
	if (i<NUM_SIGNALS && i >= 0){
		// dsp_signal_lookup_managed[i].index
		return (const gchar*)dsp_signal_lookup_managed[i].label;
	} else
		return "INVALID INDEX";
}

const gchar *sranger_mk3_hwi_dev::lookup_signal_unit_by_index(int i){
	if (i<NUM_SIGNALS && i >= 0)
		return (const gchar*)dsp_signal_lookup_managed[i].unit;
	else
		return "INVALID INDEX";
}

double sranger_mk3_hwi_dev::lookup_signal_scale_by_index(int i){
	if (i<NUM_SIGNALS && i >= 0)
		return dsp_signal_lookup_managed[i].scale;
	else
		return 0.;
}

int sranger_mk3_hwi_dev::change_signal_input(int signal_index, gint32 input_id, gint32 voffset){
	gint32 si = signal_index | (voffset >= 0 && voffset < dsp_signal_lookup_managed[signal_index].dim ? voffset<<16 : 0); 
	SIGNAL_MANAGE sm = { input_id, si, 0, 0 }; // for read/write control part of signal_monitor only
	PI_DEBUG_GP (DBG_L3, "sranger_mk3_hwi_dev::change_module_signal_input\n");

	CONV_32 (sm.mindex);
	CONV_32 (sm.signal_id);     
	CONV_32 (sm.act_address_input_set); 
	CONV_32 (sm.act_address_signal);
	lseek (dsp, magic_data.signal_monitor, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_write (dsp, &sm, sizeof (sm)); 

	PI_DEBUG_GP (DBG_L3, "sranger_mk3_hwi_dev::change_module_signal_input done: [%d,%d,0x%x,0x%x]\n",
                     sm.mindex,sm.signal_id,sm.act_address_input_set,sm.act_address_signal);
	return 0;
}

int sranger_mk3_hwi_dev::query_module_signal_input(gint32 input_id){
        int mode=0;
        int ret;
	SIGNAL_MANAGE sm = { input_id, -1, 0, 0 }; // for read/write control part of signal_monitor only
	PI_DEBUG_GP (DBG_L4, "sranger_mk3_hwi_dev::query_module_signal_input_config\n");

	CONV_32 (sm.mindex);
	CONV_32 (sm.signal_id);     
	CONV_32 (sm.act_address_input_set); 
	CONV_32 (sm.act_address_signal);

        // must protect write/read pairs from eventual multitasking/job interleaves
	lseek (dsp, magic_data.signal_monitor, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
        ret =  ioctl (dsp, SRANGER_MK23_IOCTL_SET_EXCLUSIVE_AUTO, (unsigned long)&mode);
        //g_message ("sranger_mk3_hwi_dev::query_module_signal_input: SRANGER_MK23_IOCTL_SET_EXCLUSIVE_AUTO ret=%d mode=%d ", ret, mode);
        
        //	lseek (dsp, magic_data.signal_monitor, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC | SRANGER_MK23_SEEK_EXCLUSIVE_AUTO);
	sr_write (dsp, &sm, sizeof (sm)); 

        ret =  ioctl (dsp, SRANGER_MK23_IOCTL_QUERY_EXCLUSIVE_MODE, (unsigned long)&mode);
        //g_message ("sranger_mk3_hwi_dev::query_module_signal_input: (post write) EXCLUSIVE_AUTO ret=%d mode=%d ", ret, mode);

	usleep (10000);

	lseek (dsp, magic_data.signal_monitor, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
        //	lseek (dsp, magic_data.signal_monitor, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC | SRANGER_MK23_SEEK_EXCLUSIVE_AUTO);
	sr_read (dsp, &sm, sizeof (sm)); 

        ret =  ioctl (dsp, SRANGER_MK23_IOCTL_QUERY_EXCLUSIVE_MODE, (unsigned long)&mode);
        //g_message ("sranger_mk3_hwi_dev::query_module_signal_input: (post read) EXCLUSIVE_AUTO ret=%d mode=%d ", ret, mode);

	CONV_32 (sm.mindex);
	CONV_32 (sm.signal_id);     
	CONV_32 (sm.act_address_input_set); 
	CONV_32 (sm.act_address_signal);

        if (sm.act_address_signal == 0) // disabled signal?
                return SIGNAL_INPUT_DISABLED;
        
	int signal_index = lookup_signal_by_ptr (sm.act_address_signal);

	PI_DEBUG_PLAIN (DBG_L2,
                        "Query_Module_Signal[inpid=" << input_id << "]: found Sig " << std::setw(3) << signal_index << " [v" << dsp_signal_lookup_managed[signal_index].index << "] "
                        << " *** mindex=" << sm.mindex
                        << ", signal_id=" << sm.signal_id
                        << ", act_address_input_set=" << std::hex << sm.act_address_input_set << std::dec
                        << ", act_address_input_signal=" << std::hex << sm.act_address_signal << std::dec
                        << "\n"
                        );

	return signal_index;
}

int sranger_mk3_hwi_dev::read_signal_lookup (){
	off_t adr;
	struct{ DSP_INT32_P p; } dsp_signal_list[NUM_SIGNALS];
	adr = magic_data.signal_lookup; 
	PI_DEBUG_GP (DBG_L4, "sranger_mk3_hwi_dev::read_signal_lookup at 0x%08x size=%lu\n", adr, sizeof (dsp_signal_list));
	lseek (dsp, adr, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_read (dsp, &dsp_signal_list, sizeof (dsp_signal_list)); 

	for (int i=0; i<NUM_SIGNALS; ++i){
		CONV_32 (dsp_signal_list[i].p);
		dsp_signal_lookup_managed[i].p = dsp_signal_list[i].p;
		dsp_signal_lookup_managed[i].dim   = dsp_signal_lookup[i].dim;
		dsp_signal_lookup_managed[i].label = g_strdup(dsp_signal_lookup[i].label);
                if (i==0){ // IN0 dedicated to tunnel current via IVC
                        // g_print ("1nA to Volt=%g  1pA to Volt=%g",main_get_gapp()->xsm->Inst->nAmpere2V (1.),main_get_gapp()->xsm->Inst->nAmpere2V (1e-3));
                        if (main_get_gapp()->xsm->Inst->nAmpere2V (1.) > 1.){
                                dsp_signal_lookup_managed[i].unit  = g_strdup("pA"); // use pA scale
                                dsp_signal_lookup_managed[i].scale = dsp_signal_lookup[i].scale; // -> Volts
                                dsp_signal_lookup_managed[i].scale /= main_get_gapp()->xsm->Inst->nAmpere2V (1); // values are always in nA
                        } else {
                                dsp_signal_lookup_managed[i].unit  = g_strdup("nA");
                                dsp_signal_lookup_managed[i].scale = dsp_signal_lookup[i].scale; // -> Volts
                                dsp_signal_lookup_managed[i].scale /= main_get_gapp()->xsm->Inst->nAmpere2V (1.); // nA
                        }
                } else {
                        dsp_signal_lookup_managed[i].unit  = g_strdup(dsp_signal_lookup[i].unit);
                        dsp_signal_lookup_managed[i].scale = dsp_signal_lookup[i].scale;
                }
		dsp_signal_lookup_managed[i].module  = g_strdup(dsp_signal_lookup[i].module);
		dsp_signal_lookup_managed[i].index  = -1;
                PI_DEBUG_PLAIN (DBG_L2,
                                "Sig[" << i << "]: ptr=" << dsp_signal_lookup_managed[i].p 
                                << ", " << dsp_signal_lookup_managed[i].dim 
                                << ", " << dsp_signal_lookup_managed[i].label 
                                << " [v" << dsp_signal_lookup_managed[i].index << "] "
                                << ", " << dsp_signal_lookup_managed[i].unit
                                << ", " << dsp_signal_lookup_managed[i].scale
                                << ", " << dsp_signal_lookup_managed[i].module
                                << "\n"
                                );
		for (int k=0; k<i; ++k)
			if (dsp_signal_lookup_managed[k].p == dsp_signal_lookup_managed[i].p){
                                PI_DEBUG_PLAIN (DBG_L2,
                                                "Sig[" << i << "]: ptr=" << dsp_signal_lookup_managed[i].p 
                                                << " identical with Sig[" << k << "]: ptr=" << dsp_signal_lookup_managed[k].p 
                                                << " ==> POSSIBLE ERROR IN SIGNAL TABLE <== GXSM is aborting here, suspicious DSP data.\n"
                                                );
                                g_warning ("DSP SIGNAL TABLE finding: Sig[%d] '%s': ptr=%x is identical with Sig[%d] '%s': ptr=%x",
                                           i, dsp_signal_lookup_managed[i].label, dsp_signal_lookup_managed[i].p,
                                           k, dsp_signal_lookup_managed[k].label, dsp_signal_lookup_managed[k].p);
				// exit (-1);
			}
	}
	return 0;
}

int sranger_mk3_hwi_dev::read_actual_module_configuration (){
	typedef struct { gint32 id; const gchar* name; DSP_INT32_P sigptr; } MOD_INPUT;
	MOD_INPUT mod_input_list[] = {
		//## [ MODULE_SIGNAL_INPUT_ID, name, actual hooked signal address ]
		{ DSP_SIGNAL_Z_SERVO_INPUT_ID, "Z_SERVO", 0 },
		{ DSP_SIGNAL_M_SERVO_INPUT_ID, "M_SERVO", 0 },
		{ DSP_SIGNAL_MIXER0_INPUT_ID, "MIXER0", 0 },
		{ DSP_SIGNAL_MIXER1_INPUT_ID, "MIXER1", 0 },
		{ DSP_SIGNAL_MIXER2_INPUT_ID, "MIXER2", 0 },
		{ DSP_SIGNAL_MIXER3_INPUT_ID, "MIXER3", 0 },

		{ DSP_SIGNAL_DIFF_IN0_ID, "DIFF_IN0", 0 },
		{ DSP_SIGNAL_DIFF_IN1_ID, "DIFF_IN1", 0 },
		{ DSP_SIGNAL_DIFF_IN2_ID, "DIFF_IN2", 0 },
		{ DSP_SIGNAL_DIFF_IN3_ID, "DIFF_IN3", 0 },

		{ DSP_SIGNAL_SCAN_CHANNEL_MAP0_ID, "SCAN_CHMAP0", 0 },
		{ DSP_SIGNAL_SCAN_CHANNEL_MAP1_ID, "SCAN_CHMAP1", 0 },
		{ DSP_SIGNAL_SCAN_CHANNEL_MAP2_ID, "SCAN_CHMAP2", 0 },
		{ DSP_SIGNAL_SCAN_CHANNEL_MAP3_ID, "SCAN_CHMAP3", 0 },

		{ DSP_SIGNAL_LOCKIN_A_INPUT_ID, "LOCKIN_A", 0 },
		{ DSP_SIGNAL_LOCKIN_B_INPUT_ID, "LOCKIN_B", 0 },
		{ DSP_SIGNAL_VECPROBE0_INPUT_ID, "VECPROBE0", 0 },
		{ DSP_SIGNAL_VECPROBE1_INPUT_ID, "VECPROBE1", 0 },
		{ DSP_SIGNAL_VECPROBE2_INPUT_ID, "VECPROBE2", 0 },
		{ DSP_SIGNAL_VECPROBE3_INPUT_ID, "VECPROBE3", 0 },
		{ DSP_SIGNAL_VECPROBE0_CONTROL_ID, "VECPROBE0_C", 0 },
		{ DSP_SIGNAL_VECPROBE1_CONTROL_ID, "VECPROBE1_C", 0 },
		{ DSP_SIGNAL_VECPROBE2_CONTROL_ID, "VECPROBE2_C", 0 },
		{ DSP_SIGNAL_VECPROBE3_CONTROL_ID, "VECPROBE3_C", 0 },

		{ DSP_SIGNAL_OUTMIX_CH5_INPUT_ID, "OUTMIX_CH5", 0 },
		{ DSP_SIGNAL_OUTMIX_CH5_ADD_A_INPUT_ID, "OUTMIX_CH5_ADD_A", 0 },
		{ DSP_SIGNAL_OUTMIX_CH5_SUB_B_INPUT_ID, "OUTMIX_CH5_SUB_B", 0 },
		{ DSP_SIGNAL_OUTMIX_CH5_SMAC_A_INPUT_ID, "OUTMIX_CH5_SMAC_A", 0 },
		{ DSP_SIGNAL_OUTMIX_CH5_SMAC_B_INPUT_ID, "OUTMIX_CH5_SMAC_B", 0 },

		{ DSP_SIGNAL_OUTMIX_CH6_INPUT_ID, "OUTMIX_CH6", 0 },
		{ DSP_SIGNAL_OUTMIX_CH6_ADD_A_INPUT_ID, "OUTMIX_CH6_ADD_A", 0 },
		{ DSP_SIGNAL_OUTMIX_CH6_SUB_B_INPUT_ID, "OUTMIX_CH6_SUB_B", 0 },
		{ DSP_SIGNAL_OUTMIX_CH6_SMAC_A_INPUT_ID, "OUTMIX_CH6_SMAC_A", 0 },
		{ DSP_SIGNAL_OUTMIX_CH6_SMAC_B_INPUT_ID, "OUTMIX_CH6_SMAC_B", 0 },

		{ DSP_SIGNAL_OUTMIX_CH7_INPUT_ID, "OUTMIX_CH7", 0 },
		{ DSP_SIGNAL_OUTMIX_CH7_ADD_A_INPUT_ID, "OUTMIX_CH7_ADD_A", 0 },
		{ DSP_SIGNAL_OUTMIX_CH7_SUB_B_INPUT_ID, "OUTMIX_CH7_SUB_B", 0 },
		{ DSP_SIGNAL_OUTMIX_CH7_SMAC_A_INPUT_ID, "OUTMIX_CH7_SMAC_A", 0 },
		{ DSP_SIGNAL_OUTMIX_CH7_SMAC_B_INPUT_ID, "OUTMIX_CH7_SMAC_B", 0 },

		{ DSP_SIGNAL_OUTMIX_CH8_INPUT_ID, "OUTMIX_CH8_INPUT", 0 },
		{ DSP_SIGNAL_OUTMIX_CH8_ADD_A_INPUT_ID, "OUTMIX_CH8_ADD_A_INPUT", 0 },
		{ DSP_SIGNAL_OUTMIX_CH9_INPUT_ID, "OUTMIX_CH9_INPUT", 0 },
		{ DSP_SIGNAL_OUTMIX_CH9_ADD_A_INPUT_ID, "OUTMIX_CH9_ADD_A_INPUT", 0 },

		{ DSP_SIGNAL_ANALOG_AVG_INPUT_ID, "ANALOG_AVG_INPUT", 0 },
		{ DSP_SIGNAL_SCOPE_SIGNAL1_INPUT_ID, "SCOPE_SIGNAL1_INPUT", 0 },
		{ DSP_SIGNAL_SCOPE_SIGNAL2_INPUT_ID, "SCOPE_SIGNAL2_INPUT", 0 },

		{ 0, "END", 0 }
	};

	for (int i=0; mod_input_list[i].id; ++i){
		int si = query_module_signal_input (mod_input_list[i].id);
		if (si >= 0 && si < NUM_SIGNALS){
                        PI_DEBUG_GP (DBG_L2, "INPUT %s (%04X) is set to %s\n",
                                     mod_input_list[i].name, mod_input_list[i].id,
                                     dsp_signal_lookup_managed[si].label );
		} else {
                        if (si == SIGNAL_INPUT_DISABLED)
                                PI_DEBUG_GP (DBG_L2, "INPUT %s (%04X) is DISABLED\n", mod_input_list[i].name, mod_input_list[i].id);
                        else
                                PI_DEBUG_GP (DBG_L2, "INPUT %s (%04X) -- ERROR DETECTED\n", mod_input_list[i].name, mod_input_list[i].id);
		}
	}

	return 0;
}

/* END */
