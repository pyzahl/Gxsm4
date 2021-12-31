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


#include <locale.h>
#include <libintl.h>
#include <time.h>

#include <climits>    // for limits macrors
#include <iomanip>    // for setfill, setprecision, setw
#include <ios>        // for dec, hex, oct, showbase, showpos, etc.
#include <iostream>   // for cout
#include <sstream>    // for istringstream

#include <fcntl.h>
#include <sys/ioctl.h>

#include "core-source/glbvars.h"
#include "core-source/xsmtypes.h"
#include "core-source/xsmdebug.h"

#include "sranger_mk2_hwi.h"

// you may want to handle/emulate some DSP commands later...
#include "dsp-pci32/xsm/dpramdef.h"
#include "dsp-pci32/xsm/xsmcmd.h"


// need some SRanger io-controls 
// HAS TO BE IDENTICAL TO THE DRIVER's FILE!
#include "modules/sranger_mk23_ioctl.h"

// SRanger data exchange structs and consts
#include "MK2-A810_spmcontrol/FB_spm_dataexchange.h" 

// enable debug:
#define	SRANGER_DEBUG(S) XSM_DEBUG (DBG_L4, S)
#define	SRANGER_ERROR(S) XSM_DEBUG_ERROR (DBG_L4, S)


#define SR_READFIFO_RESET -1
#define SR_EMPTY_PROBE_FIFO -2

#define CONST_DSP_F16 65536.
#define VOLT2AIC(U)   (int)(gapp->xsm->Inst->VoltOut2Dig (gapp->xsm->Inst->BiasV2V (U)))
#define DVOLT2AIC(U)  (int)(gapp->xsm->Inst->VoltOut2Dig ((U)/gapp->xsm->Inst->BiasGainV2V ()))

extern int developer_option;
extern int pi_debug_level;

extern DSPControl *DSPControlClass;
extern GxsmPlugin sranger_mk2_hwi_pi;

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


gpointer FifoReadThread (void *ptr_sr);
gpointer ProbeFifoReadThread (void *ptr_sr);
gpointer ProbeFifoReadFunction (void *ptr_sr, int dspdev);



/* Construktor for Gxsm sranger support base class
 * ==================================================
 * - open device
 * - init things
 * - ...
 */
sranger_mk2_hwi_dev::sranger_mk2_hwi_dev(){
	gchar *tmp;
	SRANGER_DEBUG("open driver");
	PI_DEBUG_GP (DBG_L1, "HWI-DEV-MK2-I** HwI SR-MK2 probing\n");
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
	sranger_mark_id = -1;

	dsp=0;
	thread_dsp=0;
	probe_thread_dsp=0;
	dsp_alternative=0;

	bz_statistics[0]=0;
	bz_statistics[1]=0;
	bz_statistics[2]=0;
	bz_statistics[3]=0;
	bz_statistics[4]=0;
	PI_DEBUG_GP (DBG_L1, "Checking on xsmres.DSPDev = %s\n", xsmres.DSPDev);
	gint srdev_index_start = 0;
	if (strrchr (xsmres.DSPDev, '_') != NULL)
		srdev_index_start = atoi ( strrchr (xsmres.DSPDev, '_')+1); // start at given device, keep looking for higher numbers
	while (srdev_index_start >= 0 && srdev_index_start < 8) {
                if (!strcmp (xsmres.DSPDev, "/dev/shm/sranger_mk3emu"))
                        srdev_index_start = 8; // no more, try just this
                else
		        sprintf (xsmres.DSPDev,"/dev/sranger_mk2_%d", srdev_index_start); // override
		PI_DEBUG_GP (DBG_L1, "Looking for MK2 with runnign GXSM compatible FB_spmcontrol DSP code starting at %s\n", xsmres.DSPDev);
	  
		if((dsp = open (xsmres.DSPDev, O_RDWR)) <= 0){
		        PI_DEBUG_GP (DBG_L1, "HWI-DEV-MK2-E01-- can not open device >%s<, please check permissions. \nError: %s\n",
				 xsmres.DSPDev, strerror(errno));
			PI_DEBUG_GP (DBG_L1, "HWI-DEV-MK2-E01-- can not open device >%s<, please check permissions.\n", xsmres.DSPDev);
			if (++srdev_index_start < 8){
				PI_DEBUG_GP (DBG_L1, "... continue looking for MK2 or Mk3 device with valid SPM Control software running.\n");
				continue;
			}
			SRANGER_ERROR(
				      "Can´t open final device ID looking for MK2/Mk3: <" << xsmres.DSPDev << ">, reason: " << strerror(errno) << std::endl
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
			gapp->alert (N_("No Hardware"), N_("Open Device failed."), productid, 1);
			dsp = 0;
			return;
		}
		else{
		        int ret;
			unsigned int vendor, product;
			if ((ret=ioctl(dsp, SRANGER_MK2_IOCTL_VENDOR, (unsigned long)&vendor)) < 0){
			        SRANGER_ERROR(strerror(ret) << " cannot query VENDOR" << std::endl
					      << "Device: " << xsmres.DSPDev);
				g_free (productid);
				productid=g_strdup_printf ("Device used: %s\n Start 'gxsm4 -h no' to correct the problem.", xsmres.DSPDev);
				gapp->alert (N_("Unkonwn Hardware"), N_("Query Vendor ID failed."), productid, 1);
				PI_DEBUG_GP (DBG_L1, "HWI-DEV-MK2-E02-- unkown hardware, vendor ID mismatch.\n");
				close (dsp);
				dsp = 0;
				if (srdev_index_start < 7){
					PI_DEBUG_GP (DBG_L1, "... continue looking for MK2 or Mk3 device with valid SPM Control software running.\n");
					continue;
				}
				return;
			}
			if ((ret=ioctl(dsp, SRANGER_MK2_IOCTL_PRODUCT, (unsigned long)&product)) < 0){
			        SRANGER_ERROR(strerror(ret) << " cannot query PRODUCT" << std::endl
					      << "Device: " << xsmres.DSPDev);
				g_free (productid);
				productid=g_strdup_printf ("Device used: %s\n Start 'gxsm4 -h no' to correct the problem.", xsmres.DSPDev);
				gapp->alert (N_("Unkonwn Hardware"), N_("Query Product ID failed."), productid, 1);
				PI_DEBUG_GP (DBG_L1, "HWI-DEV-MK2-E03-- unkown hardware, product ID mismatch.\n");
				close (dsp);
				dsp = 0;
				return;
			}
			if (vendor == 0x0a59 || vendor == 0x1612){
			        g_free (productid);
				if (vendor == 0x0a59 && product == 0x0101){
				        productid=g_strdup ("Vendor/Product: B.Paillard, Signal Ranger STD");
					gapp->alert (N_("Wrong Hardware detected"), N_("No MK2 found."), productid, 1);
					PI_DEBUG_GP (DBG_L1, "HWI-DEV-MK2-E04-- wrong hardware: SR-STD, please use SR-SP2/STD HwI.\n");
					target = SR_HWI_TARGET_C54;
					close (dsp);
					dsp=0;
					exit (-1);
					return;
				}
				else if (vendor == 0x0a59 && product == 0x0103){
			    	        productid=g_strdup ("Vendor/Product: B.Paillard, Signal Ranger SP2");
					gapp->alert (N_("Wrong Hardware detected"), N_("No MK2 found."), productid, 1);
					PI_DEBUG_GP (DBG_L1, "HWI-DEV-MK2-E05-- wrong hardware: SR-SP2, please use SR-SP2/STD HwI.\n");
					target = SR_HWI_TARGET_C54;
					close (dsp);
					dsp=0;
					exit (-1);
				return;
				}else if (vendor == 0x1612 && product == 0x0100){
				        productid=g_strdup ("Vendor/Product: B.Paillard, Signal Ranger MK2 (1612:0100)");
					PI_DEBUG_GP (DBG_L1, "HWI-DEV-MK2-I01-- SR-MK2 1612:0100 found.\n");
					target = SR_HWI_TARGET_C55;
					sranger_mark_id = 2;
				}else if (vendor == 0x1612 && product == 0x0101){
				        productid=g_strdup ("Vendor/Product: B.Paillard, Signal Ranger MK2-NG (1612:0101)");
					PI_DEBUG_GP (DBG_L1, "HWI-DEV-MK2-I02-- SR-MK2-NG 1612:0101 found.\n");
					target = SR_HWI_TARGET_C55;
					sranger_mark_id = 2;
				}else if (vendor == 0x1612 && product == 0x0103){
				        productid=g_strdup ("Vendor/Product: B.Paillard, Signal Ranger MK3 (1612:0103)");
					PI_DEBUG_GP (DBG_L1, "HWI-DEV-MK2-I03-- SR-MK3-NG 1612:0103 found -- reassigning...\n");
					target = SR_HWI_TARGET_C55;
					sranger_mark_id = 3;
					close (dsp);
					dsp=0;
					return;
				}else{
				        productid=g_strdup ("Vendor/Product: B.Paillard, unkown version!");
					gapp->alert (N_("Unkonwn Hardware detected"), N_("No Signal Ranger found."), productid, 1);
					PI_DEBUG_GP (DBG_L1, "HWI-DEV-MK2-E06-- SR-MK?? 1612:%s found -- not supported.\n", productid);
					close (dsp);
					dsp=0;
					return;
				}
				
				PI_DEBUG_GP (DBG_L1, "HWI-DEV-MK2-I** reading DSP magic informations and code version.\n");
				// now read magic struct data
				lseek (dsp, FB_SPM_MAGIC_ADR, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
				sr_read (dsp, &magic_data, sizeof (magic_data)); 
				swap_flg = 0;
				PI_DEBUG_GP (DBG_L1, "HWI-DEV-MK2-I*CPU* DSP-MAGIC  : %04x   [%04x]\n", magic_data.magic, FB_SPM_MAGIC);
				if (magic_data.magic != FB_SPM_MAGIC){
				        PI_DEBUG_GP (DBG_L1, "HWI-DEV-MK2-I** checking for swap match.\n");
					swap (&magic_data.magic);
					PI_DEBUG_GP (DBG_L1, "HWI-DEV-MK2-I*BIG* DSP-MAGIC  : %04x   [%04x]\n", magic_data.magic, FB_SPM_MAGIC);
					if (magic_data.magic != FB_SPM_MAGIC){
					  
					        std::cout << "Wrong DSP magic, checking for old DSP magic location..." << std::endl;
						PI_DEBUG_GP (DBG_L1, "HWI-DEV-MK2-ME01-- wrong magic/location/version.\n");
						// Test for old DSP code at 0x4000 now read magic struct data
						lseek (dsp, 0x4000, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
						sr_read (dsp, &magic_data, sizeof (magic_data)); 
						swap_flg = 0;
						if (magic_data.magic != FB_SPM_MAGIC){
						        swap (&magic_data.magic);
							if (magic_data.magic == FB_SPM_MAGIC){
							        std::cout << "WARNING: Found old DSP code and magic (SWAP) and layout please update DSP code!" << std::endl;
								PI_DEBUG_GP (DBG_L1, "HWI-DEV-MK2-I** swap match OK.\n");
								goto old_magic_swap;
							}
						} else {
						        std::cout << "WARNING: Found old DSP code and magic and layout please update DSP code!" << std::endl;
							PI_DEBUG_GP (DBG_L1, "HWI-DEV-MK2-I** non swap match OK (running PPC host platform?).\n");
							goto old_magic;
						}
						
						SRANGER_ERROR ("DSP SPM soft cannot be identified: Magic unkown");
						productid=g_strdup_printf ("Bad Magic: %04x\n"
									   "Please launch the correct DSP software before restarting GXSM.",
									   magic_data.magic);
						gapp->alert (N_("Wrong DSP magic"), N_("DSP software was not identified.\nContinue searching..."), productid, 1);
						std::cout << "Wrong DSP magic: DSP SPM software was not identified." << productid << std::endl;
						std::cout << 
						  "* Please Load/Flash the correct SPM code into the MK2-A810 and try again.\n"
						  "* If you need to start GXSM temporary without hardware for adjusting\n"
						  "* preferences or daya analysis only, use this command:\n"
						  "* $ gxsm -h no\n"
						  "----------------------------------------------------- exiting now."
							  << std::endl;
						PI_DEBUG_GP (DBG_L1, "HWI-DEV-MK2-E08-- magic match faild.\n");
						close (dsp);
						dsp=0;
						magic_data.magic = 0; // set to zero, this means all data is invalid!

						if (++srdev_index_start < 8)
							continue; // auto attempt to try next board if any
						exit (-1);
						return;
					}
				old_magic_swap:
					PI_DEBUG_GP (DBG_L1, "HWI-DEV-MK2-ME02-- old magic swap.\n");
					// no swapp all to fix endianess
					swap_flg = 1;
					swap (&magic_data.version);
					swap (&magic_data.year);
					swap (&magic_data.mmdd);
					swap (&magic_data.dsp_soft_id);
					swap (&magic_data.statemachine);
					swap (&magic_data.AIC_in);
					swap (&magic_data.AIC_out);
					swap (&magic_data.AIC_reg);
					swap (&magic_data.feedback);
					swap (&magic_data.analog);
					swap (&magic_data.scan);
					swap (&magic_data.move);
					swap (&magic_data.probe);
					swap (&magic_data.autoapproach);
					swap (&magic_data.datafifo);
					swap (&magic_data.probedatafifo);
					swap (&magic_data.data_sync_io);
					swap (&magic_data.feedback_mixer);
					swap (&magic_data.CR_out_puls);
					swap (&magic_data.external);
					swap (&magic_data.CR_generic_io);
				}
				
			old_magic:
				SRANGER_DEBUG ("SRanger, FB_SPM soft: Magic data OK");
				PI_DEBUG_GP (DBG_L1, "HWI-DEV-MK2-I**-- magic data OK.\n");
				
                                {
                                        gchar *InfoString = g_strdup_printf("SR-MK2 connect at %s"
                                                                            "\nDSP-SoftId : %04x [HwI:%04x]"
                                                                            "\nDSP-SoftVer: %04x [HwI:%04x]"
                                                                            "\nDSP-SoftDat: %04x %04x [HwI:%04x %04x]",
                                                                            xsmres.DSPDev,
                                                                            magic_data.dsp_soft_id, FB_SPM_SOFT_ID,
                                                                            magic_data.version, FB_SPM_VERSION,
                                                                            magic_data.mmdd, magic_data.year, FB_SPM_DATE_MMDD, FB_SPM_DATE_YEAR
                                                                            );
                                        gapp->monitorcontrol->LogEvent ("MK2 HwI identification data", InfoString);
                                        g_free (InfoString);
                                }

				if (FB_SPM_SOFT_ID != magic_data.dsp_soft_id){
				        gchar *details = g_strdup_printf(
									 "Bad SR DSP Software ID %4X found.\n"
									 "SR DSP Software ID %4X expected.\n"
									 "This non SPM DSP code will not work for GSXM, cannot proceed."
									 "Please launch the correct DSP software before restarting GXSM.",
									 (unsigned int) magic_data.dsp_soft_id,
									 (unsigned int) FB_SPM_SOFT_ID);
					SRANGER_DEBUG ("Signal Ranger FB_SPM soft Version mismatch\n" << details);
					gapp->alert (N_("Critical Warning:"), N_("Signal Ranger MK2 FB_SPM software version mismatch detected! Exiting required."), details,
                                                     developer_option == 0 ? 20 : 5);
                                        gapp->monitorcontrol->LogEvent ("Critical: Signal Ranger MK2 FB_SPM soft Version mismatch:\n", details);
                                        g_critical ("Signal Ranger MK2 FB_SPM soft Version mismatch:\n%s", details);



					gapp->alert (N_("Warning"), N_("Signal Ranger FB_SPM software version mismatch detected!"), details, 1);
					PI_DEBUG_GP (DBG_L1, "HWI-DEV-MK2-VE01-- old magic -- DSP software version mismatch.\n");
					g_free (details);
					close (dsp);
					dsp=0;
					magic_data.magic = 0; // set to zero, this means all data is invalid!
					exit (-1);
					return;
				}
				
				PI_DEBUG_GP (DBG_L1, "HWI-DEV-MK2-I*CPU* DSP-Version: %04x   [HwI:%04x]\n", magic_data.version, FB_SPM_VERSION);
				if (FB_SPM_VERSION != magic_data.version || 
				    FB_SPM_SOFT_ID != magic_data.dsp_soft_id){
				        gchar *details = g_strdup_printf(
									 "Detected SRanger DSP Software Version: %x.%02x\n"
									 "GXSM was build for DSP Software Version: %x.%02x\n\n"
									 "Note: This may cause incompatility problems and unpredictable toubles,\n"
									 "however, trying to proceed in case you know what you are doing.\n\n"
									 "SwapFlag: %d   Target: %d\n",
									 magic_data.version >> 8, 
									 magic_data.version & 0xff,
									 FB_SPM_VERSION >> 8, 
									 FB_SPM_VERSION & 0xff,
									 swap_flg, target);
					PI_DEBUG_GP (DBG_L1, "HWI-DEV-MK2-VW01-- DSP software version mismatch warning.\n%s\n", details);
					gapp->alert (N_("Warning"), N_("Signal Ranger FB_SPM software version mismatch detected!"), details, 1);
					g_free (details);
				}
				
				SRANGER_DEBUG ("ProductId:" << productid);

				// open some more DSP connections, used by threads
				if((thread_dsp = open (xsmres.DSPDev, O_RDWR)) <= 0){
				        SRANGER_ERROR ("cannot open thread SR connection, trying to continue...");
					PI_DEBUG_GP (DBG_L1, "HWI-DEV-MK2-OTE01-- open multi dev error, can not generate thread DSP link.\n");
					thread_dsp = 0;
				}
				// testing...
				if((probe_thread_dsp = open (xsmres.DSPDev, O_RDWR)) <= 0){
				        SRANGER_ERROR ("cannot open probe thread SR connection, trying to continue...");
					PI_DEBUG_GP (DBG_L1, "HWI-DEV-MK2-OTE02-- open multi dev error, can not generate probe DSP link.\n");
					probe_thread_dsp = 0;
				}
				if((dsp_alternative = open (xsmres.DSPDev, O_RDWR)) <= 0){
				        SRANGER_ERROR ("cannot open alternative SR connection, trying to continue...");
					PI_DEBUG_GP (DBG_L1, "HWI-DEV-MK2-OTE03-- open multi dev error, can not generate alternate DSP link.\n");
					dsp_alternative = 0;
				}
			}else{
			        SRANGER_ERROR ("unkown vendor, exiting");
				PI_DEBUG_GP (DBG_L1, "HWI-DEV-MK2-E07-- Unkown Device Vendor ID.\n");
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

		if (f){
			g_string_append_printf (details, "\n\nPLEASE VERIFY YOUR PREFERENCES!");
			gapp->alert (N_("Warning"), N_("MK2-A810 DataAq Preferences Setup Verification"), details->str, 1);
			PI_DEBUG_GP (DBG_L1, "HWI-DEV-MK2-WW00A-- configuration/preferences mismatch with current hardware setup -- please adjust.\n");
		}
		g_string_free(details, TRUE); 
		details=NULL;
	}
}

/* Destructor
 * close device
 */
sranger_mk2_hwi_dev::~sranger_mk2_hwi_dev(){
	SRANGER_DEBUG ("closing connection to SRanger driver");
	PI_DEBUG_GP (DBG_L1, "HWI-DEV-MK2-BYBY -- closing all DSP links.\n");
	if (dsp) close (dsp);
	if (thread_dsp) close (thread_dsp);
	if (probe_thread_dsp) close (probe_thread_dsp);
	if (dsp_alternative) close (dsp_alternative);
	if (productid) g_free (productid);
}

// data translation helpers

void sranger_mk2_hwi_dev::swap (guint16 *addr){
	guint16 temp1, temp2;
	temp1 = temp2 = *addr;
	*addr = ((temp2 & 0xFF) << 8) | ((temp1 >> 8) & 0xFF);
}

void sranger_mk2_hwi_dev::swap (gint16 *addr){
	guint16 temp1, temp2;
	temp1 = temp2 = *addr;
	*addr = ((temp2 & 0xFF) << 8) | ((temp1 >> 8) & 0xFF);
}

void sranger_mk2_hwi_dev::swap (gint32 *addr){
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

void sranger_mk2_hwi_dev::swap (guint32 *addr){
	guint32 temp1, temp2, temp3, temp4;

	temp1 = (*addr)       & 0xFF;
	temp2 = (*addr >> 8)  & 0xFF;
	temp3 = (*addr >> 16) & 0xFF;
	temp4 = (*addr >> 24) & 0xFF;


	if (target == SR_HWI_TARGET_C55)
		*addr = (temp3 << 24) | (temp4 << 16) | (temp1 << 8) | temp2;
	else // can assume so far else is: if (target == SR_HWI_TARGET_C54)
		*addr = (temp1 << 24) | (temp2 << 16) | (temp3 << 8) | temp4;
}

/*
Qm.n  (m+n+1)bits total, n=fractional bits, n=0: integer

Float to Q
To convert a number from floating point to Qm.n format:
Multiply the floating point number by 2^n
Round to the nearest integer

Q to Float
To convert a number from Qm.n format to floating point:
Convert the number to floating point as if it were an integer
Multiply by 2^(-n)
*/

// 1<<15 = 32768    stay away 1 from limits to avoid troubles!
gint32 sranger_mk2_hwi_dev::float_2_sranger_q15 (double x){
	if (x >= 1.)
		return 32767;
	if (x <= -1.)
		return -32766;
	
	return (gint32)round(x*32767.);
}

// 1<<31 = 2147483648
gint32 sranger_mk2_hwi_dev::float_2_sranger_q31 (double x){
	if (x >= 1.)
		return 0x7FFFFFFF;
	if (x <= -1.)
		return -0x7FFFFFFE;
	
	return (gint32)round(x*2147483647.);
}

gint32 sranger_mk2_hwi_dev::int_2_sranger_int (gint32 x){
	gint16 tmp = x > 32767 ? 32767 : x < -32766 ? -32766 : x; // saturate
	if (swap_flg)
		swap (&tmp);
	return (gint32)tmp;
}

guint16 sranger_mk2_hwi_dev::uint_2_sranger_uint (guint16 x){
	guint16 tmp = x;
	if (swap_flg)
		swap (&tmp);
	return tmp;
}

gint32 sranger_mk2_hwi_dev::long_2_sranger_long (gint32 x){
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
}


guint32 sranger_mk2_hwi_dev::ulong_2_sranger_ulong (guint32 x){
	if (swap_flg){
		swap (&x);
		return x;
	}

	if (target == SR_HWI_TARGET_C55){
		guint32 temp0, temp1, temp2, temp3;
		gchar  x32[4];
		
		
		temp0 = (x)       & 0xFF;
		temp1 = (x >> 8)  & 0xFF;
		temp2 = (x >> 16) & 0xFF;
		temp3 = (x >> 24) & 0xFF;

		x32[0] = temp2;
		x32[1] = temp3;
		x32[2] = temp0;
		x32[3] = temp1;

		x = *((guint32*)x32);
		
		return x;
	} else
		return x;
}




// Image Data FIFO read thread section
// ==================================================

// start_fifo_read:
// Data Transfer Setup:
// Prepare and Fork Image Data FIFO read thread

int sranger_mk2_hwi_dev::start_fifo_read (int y_start, 
					  int num_srcs0, int num_srcs1, int num_srcs2, int num_srcs3, 
					  Mem2d **Mob0, Mem2d **Mob1, Mem2d **Mob2, Mem2d **Mob3){

	PI_DEBUG_GP (DBG_L1, "HWI-DEV-MK2-DBGI mk2::start fifo read\n");

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
		fifo_read_thread = g_thread_new ("FifoReadThread", FifoReadThread, this);

		if ((DSPControlClass->Source & 0xffff) && DSPControlClass->probe_trigger_raster_points > 0){
			DSPControlClass->probe_trigger_single_shot = 0;
			ReadProbeFifo (thread_dsp, FR_FIFO_FORCE_RESET); // reset FIFO
			ReadProbeFifo (thread_dsp, FR_INIT); // init
		}
	}
	else{
		if (DSPControlClass->vis_Source & 0xffff)
			probe_fifo_read_thread = g_thread_new ("ProbeFifoReadThread", ProbeFifoReadThread, this);
	}

	return 0;
}

// FifoReadThread:
// Image Data FIFO read thread

gpointer FifoReadThread (void *ptr_sr){
	sranger_mk2_hwi_dev *sr = (sranger_mk2_hwi_dev*)ptr_sr;
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

	std::cout << "BZ DATA STATISTICS: [8: " << sr->bz_statistics[0] 
		  << ", 16:" << sr->bz_statistics[1] 
		  << ", 24:" << sr->bz_statistics[2] 
		  << ", 32:" << sr->bz_statistics[3] 
		  << ", ST:" << sr->bz_statistics[4] 
		  << "]" << std::endl;
	

	return NULL;
}

// ReadLineFromFifo:
// read scan line from FIFO -- wait for data, and empty FIFO as quick as possible, 
// sort data chunks away int scan-mem2d objects
int sranger_mk2_hwi_dev::ReadLineFromFifo (int y_index){
	static int len[4] = { 0,0,0,0 };
	static int total_len = 0;
	static int fifo_reads = 0;
	static DATA_FIFO dsp_fifo;
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
		if (DSPControlClass->probe_trigger_raster_points > 0){
			for (int i=0; i<10; ++i){ // about 1/4s finish time
				// free some cpu time now
				usleep(2500);
				ProbeFifoReadFunction (this, thread_dsp);
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

		lseek (thread_dsp, magic_data.datafifo, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
		sr_read (thread_dsp, &dsp_fifo, (MAX_WRITE_DATA_FIFO+3)<<1);
		dsp_fifo.stall = 0; // unlock scanning
		check_and_swap (dsp_fifo.stall);
		sr_write (thread_dsp, &dsp_fifo, (MAX_WRITE_DATA_FIFO+3)<<1);
		// to PC format
		check_and_swap (dsp_fifo.r_position);
		check_and_swap (dsp_fifo.w_position);
		readfifo_status = 1;
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
						if (DSPControlClass->probe_trigger_raster_points > 0)
							ProbeFifoReadFunction (this, thread_dsp);
		
						// Get all and empty fifo
						lseek (thread_dsp, magic_data.datafifo, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC); // ### CONSIDER NON_ATOMIC
						sr_read  (thread_dsp, &dsp_fifo, sizeof (dsp_fifo));
						bz_need_reswap = 1;
						++fifo_reads;
						
						// to PC format
						check_and_swap (dsp_fifo.w_position);
						check_and_swap (dsp_fifo.length); // to PC
						wpos_tmp = dsp_fifo.w_position;

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
						//						usleep(500);
					} while (fifo_fill < 10 && ScanningFlg);

					// calc and update Fifo stats
					{
						double fifo_fill_percent;
#ifdef FIFO_DBG
						if (wpos_tmp > dsp_fifo.length){
							PI_DEBUG_GP (DBG_L1, "FIFO LOOPING: w=%04x (%d) r=%04x (%d) fill=%04x (%d)  L=%x %d\n",
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
//						PI_DEBUG_GP (DBG_L1, AddStatusString);
						PI_DEBUG_GP (DBG_L1, "fifo buffer read #=%04d, w-r=%04x, dir=%d Stats: %s\n", fifo_reads, dsp_fifo.w_position - dsp_fifo.r_position, dir, AddStatusString);
#endif
					}
				}
						
				// transfer and convert data from fifo buffer
				rpos_tmp += count = FifoRead (rpos_tmp, wpos_tmp,
							      xi, fifo_data_num_srcs[dir], len[dir], 
							      linebuffer_f, dsp_fifo.buffer.w, dsp_fifo.buffer.l);
						
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
	if (DSPControlClass->probe_trigger_raster_points > 0)
		ProbeFifoReadFunction (this, thread_dsp);

	return (ScanningFlg) ? 0:1;
}

void Xdumpbuffer(unsigned char *buffer, int size, int i0){
	int i,j,in;
	in=size;
	for(i=i0; i<in; i+=16){
		g_print("W %06X:",(i>>1)&DATAFIFO_MASK);
		for(j=0; (j<16) && (i+j)<size; j++)
			g_print(" %02x",buffer[(((i>>1)&DATAFIFO_MASK)<<1) + j]);
		printf("   ");
		for(j=0; (j<16) && (i+j)<size; j++)
			if(isprint(buffer[j+i]))
				g_print("%c",buffer[(((i>>1)&DATAFIFO_MASK)<<1) + j]);
			else
				g_print(".");
		g_print("\n");
	}
}

int xindexx=0;
int yindexy=0;
int _norm(gint32 x){
	if (x != 0){
		int i=0;
		if (x<0) x=-x;
		while ((x & 0x40000000) == 0  && i < 32) { x <<= 1; ++i; }
		return i;
	} else return 32;
}

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

int _norm(gint32 x){
	if (x != 0){
		int i=0;
		if (x<0) x=-x;
		while ((x & 0x40000000) == 0  && i < 32) { x <<= 1; ++i; }
		return i;
	} else return 32;
}

void bz_push(int i, long x){
	union { DSP_LONG l; DSP_ULONG ul; } delta;
	int bits;

	bz_testdata[testindex++] = x;

// ERRORS so far to DSP code: norm == 0 test!! missed delta.ul ">>2" in add on parts below!! Fix "else" construct of 32bit full!!

	delta.l = x - bz_last[i];
	bz_last[i] = x;

	if (bz_push_mode) 
		goto bz_push_mode_32;

	bits  = _norm (delta.l); // THIS NORM RETURNs 32 for arg=0
	std::cout << "BZ_PUSH(i=" << i << ", ***x=" << std::hex << x << std::dec << ") D.l=" << delta.l << " bits=" << bits;
	if (bits > 26){ // 8 bit package, 6 bit data
		delta.l <<= 26;
		std::cout << ">26** 8:6bit** BZdelta.l=" << std::hex << delta.l << std::dec << " bz_byte_pos=" << bz_byte_pos << std::endl;;
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
		std::cout << ">18**16:14bit** BZdelta.l=" << std::hex << delta.l << std::dec << " bz_byte_pos=" << bz_byte_pos << std::endl;;
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
		std::cout << ">10**24:22bit** BZdelta.l=" << std::hex << delta.l << std::dec << " bz_byte_pos=" << bz_byte_pos << std::endl;
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
		std::cout << "ELSE**40:32bit** BZdelta.l=" << std::hex << delta.l << std::dec << " bz_byte_pos=" << bz_byte_pos << std::endl;
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

		Xdumpbuffer ((unsigned char*)&datafifo.buffer.ul[bits-4], 32, 0);
	}
//	std::cout << "----> Fin bz_byte_pos=" << bz_byte_pos << std::endl;;
}

void run_test(){
	memset ((unsigned char*)datafifo.buffer.ul, 0, DATAFIFO_LENGTH<<1);
	bz_init ();
	Xdumpbuffer ((unsigned char*)datafifo.buffer.ul, 32, 0);
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
int sranger_mk2_hwi_dev::FifoRead (int start, int end, int &xi, int num_srcs, int len, float *buffer_f, SHT *fifo_w, LNG *fifo_l){
	int count=0;
	int num_f = num_srcs&0x0f;
	int num_l = (num_srcs&0xf0)>>4;
	int fi_delta = end-start;
 // -- DSP pads at end of scan with full 32-bit 3F00000000 x MAX_CHANNELS -- so packed record is never longer than this:
	const int max_length = ((5*(num_srcs+2))>>1);

//	PI_DEBUG_GP (DBG_L1, "FIFO read attempt: FIFO BUFFER [start=%x :: end=%x]):\n", start, end);
//	Xdumpbuffer ((unsigned char*)fifo_l, (end+16)<<1, start<<1);

	while (end < start) end += DATAFIFO_LENGTH;

	end -= max_length;

// make sure we have at least max_length data ready before proceeding
// -- need to have at least one full record, of unkown length, but smaller than the end pad.
	if (start > end)
		return 0;

	if (fifo_block == 0) yindexy=0;
	++fifo_block;

//	PI_DEBUG_GP (DBG_L1, "FIFO read[%6d]( xi=%d numsrcs=%d, len=%d FIFO BUFFER CIRC [start=%x.%d :: end=%x]):\n", fifo_block, xi, num_srcs, len, start, bz_byte_pos, end);
	


#ifdef BZ_TEST
	int swfold;
//	Xdumpbuffer ((unsigned char*)fifo_l, (end-start)<<1, start<<1);

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
	Xdumpbuffer ((unsigned char*)fifo_l, end, 0);

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
			PI_DEBUG_GP (DBG_L1, "FIFO top of buffer at fi=0x%04x / count=%d\n-------- Fifo Debug:\n", fi, count);
			PI_DEBUG_GP (DBG_L1, "FIFO read[%6d]( xi=%d numsrcs=%d, len=%d FIFO BUFFER CIRC [start=%x.%d :: end=%x]):\n", fifo_block, xi, num_srcs, len, start, bz_byte_pos, end);
			Xdumpbuffer ((unsigned char*)fifo_l, (end+16)<<1, start<<1);
		}
#endif

		xindexx = xi; // dbg only

		for (int i=0; i<(num_f+num_l); ++i){
			if (fi >= (end+max_length)){
				PI_DEBUG_GP (DBG_L1, "ERROR buffer valid range overrun: i of num_f=%d fi=%4x, x=%d, y=%d\n-------- Fifo Debug:\n", i, fi, xindexx, yindexy);
				PI_DEBUG_GP (DBG_L1, "FIFO read[%6d]( xi=%d numsrcs=%d, len=%d FIFO BUFFER CIRC [start=%x.%d :: end=%x]):\n", fifo_block, xi, num_srcs, len, start, bz_byte_pos, end);
				Xdumpbuffer ((unsigned char*)fifo_l, (end+16)<<1, start<<1);
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
//				PI_DEBUG_GP (DBG_L1, "FIFO read: START_PACKAGE! ns=%d, srcs=%4x\n", bz_dsp_ns, bz_dsp_srcs);
					continue;
				case 0x3f:
					PI_DEBUG_GP (DBG_L1, "FIFO read: AS 0x3F END MARK DETECTED (should not get there, ERROR?)\n-------- Fifo Debug:\n");
					PI_DEBUG_GP (DBG_L1, "FIFO read[%6d]( xi=%d numsrcs=%d, len=%d FIFO BUFFER CIRC [start=%x.%d :: end=%x]):\n", fifo_block, xi, num_srcs, len, start, bz_byte_pos, end);
					Xdumpbuffer ((unsigned char*)fifo_l, (end+16)<<1, start<<1);
#if 0 // not thread save?
					{
						GtkWidget *dialog = gtk_message_dialog_new (NULL,
											    GTK_DIALOG_DESTROY_WITH_PARENT,
											    GTK_MESSAGE_WARNING,
											    GTK_BUTTONS_CLOSE,
											    "Possible FIFO overflow error detected.\n"
											    "Unexpected/early reading of 0x3F DATA END MARK.\n"
											    "FIFO read[%6d]:\nxi=%d numsrcs=%d, len=%d [start=%x.%d :: end=%x]: see console.\n\n"
											    "To recover: Please hit Stop-Scan and restart.\n"
											    "If near max USB bandwidth consider lower scan speed,\n"
											    "less data sources or eliminate any additional USB bus loads.\n",
											    fifo_block, xi, num_srcs, len, start, bz_byte_pos, end
							);
						g_signal_connect_swapped (G_OBJECT (dialog), "response",
									  G_CALLBACK (gtk_widget_destroy),
									  G_OBJECT (dialog));
						gtk_widget_show (dialog);
					}
#endif
					return count;
					break;
				default:
					PI_DEBUG_GP (DBG_L1, "FIFO read: 32 Bit Unkown Package MARK %2x DETECTED at fi=0x%04x / count=%d (ERROR?)\n-------- Fifo Debug:\n", bz_push_mode, fi, count);
					PI_DEBUG_GP (DBG_L1, "FIFO read[%6d]( xi=%d numsrcs=%d, len=%d FIFO BUFFER CIRC [start=%x.%d :: end=%x]):\n", fifo_block, xi, num_srcs, len, start, bz_byte_pos, end);
					Xdumpbuffer ((unsigned char*)fifo_l, (end+16)<<1, start<<1);
					return count;
					break;
				}
			if (i==0 && bz_dsp_srcs&1) // put 16.16 Topo data chunks from packed stream
				buffer_f[xi+i*len] = (float)((double)x/65536.);
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
			++yindexy;
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
	PI_DEBUG_GP (DBG_L1, "FIFO read done[y=%d]. Count=%x.%d\n", yindexy, count, bz_byte_pos);
	for (int j=0; j<num_f; ++j){
		PI_DEBUG_GP (DBG_L1, "Buffer[%d,xi#%d]={",j,xi);
		for (int i=0; i<xi; ++i)
			PI_DEBUG_GP (DBG_L1, "%g,",buffer_f[i+j*len]);
		PI_DEBUG_GP (DBG_L1, "}\n");
	}
#endif
	return count;
}

LNG sranger_mk2_hwi_dev::bz_unpack (int i, LNG *fifo_l, int &fi, int &count){
	int bits_mark[4] = { 32, 10, 18, 26 };
	union { DSP_LONG l; DSP_ULONG ul; } x;
	union { DSP_LONG l; DSP_ULONG ul; } delta;
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
	
//	PI_DEBUG_GP (DBG_L1, "BZ_UNPACK(%6x,%6d) [%4d, %4d, %d] bits=%2d @ pos=%d x.ul=%08x ",fi,count,xindexx, yindexy, i, bits, bz_byte_pos, x.ul);
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
//			PI_DEBUG_GP (DBG_L1, " ... 3: x.ul=%08x", x.ul);
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
//			PI_DEBUG_GP (DBG_L1, " ... 2: x.ul=%08x", x.ul);
			delta.ul |= (x.ul & 0xff000000) >> 14; delta.l >>= 10; bz_byte_pos=1; break;
		default: delta.ul = (x.ul & 0x0000003f) << 26; fi+=2; fi&=DATAFIFO_MASK; count+=2;
			check_and_swap (fifo_l[fi>>1L]);
			x.l = fifo_l[fi>>1L];
//			PI_DEBUG_GP (DBG_L1, " ... 3: x.ul=%08x", x.ul);
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
//			PI_DEBUG_GP (DBG_L1, " bzp=%02x ... 0: x.ul=%08x", bz_push_mode, x.ul);
			delta.ul |= (x.ul & 0xff000000) >> 24; bz_byte_pos=1; break;
		case 1:  delta.ul = (x.ul & 0x0000ffff) << 16;
			bz_push_mode = (x.ul & 0x00ff0000) >> 16; fi+=2; fi&=DATAFIFO_MASK; count+=2;
			check_and_swap (fifo_l[fi>>1L]);
			x.l = fifo_l[fi>>1L];
//			PI_DEBUG_GP (DBG_L1, " bzp=%02x ... 1: x.ul=%08x", bz_push_mode, x.ul);
			delta.ul |= (x.ul & 0xffff0000) >> 16; bz_byte_pos=2; break;
		case 2:  delta.ul = (x.ul & 0x000000ff) << 24;
			bz_push_mode = (x.ul & 0x0000ff00) >> 8; fi+=2; fi&=DATAFIFO_MASK; count+=2;
			check_and_swap (fifo_l[fi>>1L]);
			x.l = fifo_l[fi>>1L];
//			PI_DEBUG_GP (DBG_L1, " bzp=%02x ... 2: x.ul=%08x", bz_push_mode, x.ul);
			delta.ul |= (x.ul & 0xffffff00) >>  8; bz_byte_pos=3; break;
		default:  	 
			bz_push_mode = (x.ul & 0x000000ff); fi+=2; fi&=DATAFIFO_MASK; count+=2;
			check_and_swap (fifo_l[fi>>1L]);
			x.l = fifo_l[fi>>1L];
//			PI_DEBUG_GP (DBG_L1, " bzp=%02x ... 3: x.ul=%08x", bz_push_mode, x.ul);
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

//	PI_DEBUG_GP (DBG_L1, "[bzp=%02x] delta.l=%08x (%6g) N[%d] ==> %08x = %6g\n", bz_push_mode, delta.l, (double)delta.l, _norm(delta.l), bz_last[i], (double)bz_last[i]);

#endif

	return bz_last[i];
}


// ==================================================
//
// Probe Data FIFO read thread section
//
// ==================================================
 
// FIFO watch verbosity...
//# define LOGMSGS0(X) std::cout << X
# define LOGMSGS0(X)

//# define LOGMSGS(X) std::cout << X
# define LOGMSGS(X)

//# define LOGMSGS2(X) std::cout << X
# define LOGMSGS2(X)

// ProbeFifoReadThread:
// Independent ProbeFifoRead Thread (manual probe)
gpointer ProbeFifoReadThread (void *ptr_sr){
	int finish_flag=FALSE;
	sranger_mk2_hwi_dev *sr = (sranger_mk2_hwi_dev*)ptr_sr;

	if (sr->probe_fifo_thread_active){
		LOGMSGS ( "ProbeFifoReadThread ERROR: Attempt to start again while in progress! [#" << sr->probe_fifo_thread_active << "]" << std::endl);
		return NULL;
	}
	sr->probe_fifo_thread_active++;
	DSPControlClass->probe_ready = FALSE;

	if (DSPControlClass->probe_trigger_single_shot)
		 finish_flag=TRUE;

	clock_t timeout = clock() + (clock_t)(CLOCKS_PER_SEC*(0.5+sr->probe_time_estimate/22000.));

	LOGMSGS ( "ProbeFifoReadThread START  " << (DSPControlClass->probe_trigger_single_shot ? "Single":"Multiple") << "-VP[#" 
		  << sr->probe_fifo_thread_active << "] Timeout is set to:" 
		  << (timeout-clock()) << "Clks (incl. 0.5s Reserve) (" << ((double)sr->probe_time_estimate/22000.) << "s)" << std::endl);

	sr->ReadProbeFifo (sr->probe_thread_dsp, FR_INIT); // init

	int i=1;
	while (sr->is_scanning () || finish_flag){
                if (DSPControlClass->current_auto_flags & FLAG_AUTO_PLOT)
                        DSPControlClass->Probing_graph_update_thread_safe ();

		++i;
		switch (sr->ReadProbeFifo (sr->probe_thread_dsp)){
		case RET_FR_NOWAIT:
			LOGMSGS2 ( ":NOWAIT:" << i << " TmoClk=" << (timeout-clock()) << std::endl);
			continue;
		case RET_FR_WAIT:
			LOGMSGS ( ":WAIT:" << i << " TmoClk=" << (timeout-clock()) << std::endl);
//			usleep(5000);
			usleep(1500);
			if (finish_flag && clock() > timeout)
			    goto error;
			continue;
		case RET_FR_OK:
			LOGMSGS2 ( ":OK:" << i << " TmoClk=" << (timeout-clock()) << std::endl);
			continue;
		case RET_FR_ERROR:
			LOGMSGS ( ":ERROR:" << i << " TmoClk=" << (timeout-clock()) << std::endl);
			goto error;
		case RET_FR_FCT_END: 
			LOGMSGS ( ":FCT_END:" << i << " TmoClk=" << (timeout-clock()) << std::endl);
			if (finish_flag){
				if (DSPControlClass->current_auto_flags & FLAG_AUTO_PLOT)
					DSPControlClass->Probing_graph_update_thread_safe (1);
				
				if (DSPControlClass->current_auto_flags & FLAG_AUTO_SAVE)
                                        DSPControlClass->Probing_save_callback (NULL, DSPControlClass);	

				goto finish;
			}
			DSPControlClass->push_probedata_arrays ();
			DSPControlClass->init_probedata_arrays ();

			// reset timeout
//			timeout = clock() + (clock_t)(CLOCKS_PER_SEC*(0.5+sr->probe_time_estimate/22000.));
			continue;
		}
		LOGMSGS ( ":FIFO THREAD ERROR DETECTION:" << i << " TmoClk=" << (timeout-clock()) << std::endl);
		goto error;
	}
finish:
	LOGMSGS ( "ProbeFifoReadThread DONE  Single-VP[#" << sr->probe_fifo_thread_active << "] Timeout left (positive is OK):" << (timeout-clock()) << std::endl);

error:
	sr->ReadProbeFifo (sr->probe_thread_dsp, FR_FINISH); // finish

	--sr->probe_fifo_thread_active;
	DSPControlClass->probe_ready = TRUE;

	return NULL;
}

// ProbeFifoReadFunction:
// inlineable ProbeFifoRead Function -- similar to the thread, is called/inlined by/into the Image Data Thread
gpointer ProbeFifoReadFunction (void *ptr_sr, int dspdev){
	static int plotted=0;
	sranger_mk2_hwi_dev *sr = (sranger_mk2_hwi_dev*)ptr_sr;

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
int sranger_mk2_hwi_dev::ReadProbeFifo (int dspdev, int control){
	int pvd_blk_size=0;
	static double pv[9];
	static int last = 0;
	static int last_read_end = 0;
	static DATA_FIFO_EXTERN_PCOPY fifo;
	static PROBE_SECTION_HEADER section_header;
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
		sr_write (dspdev, &fifo, 2*sizeof(DSP_INT)); // reset positions now to prevent reading old/last dataset before DSP starts init/putting data
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
		LOGMSGS ( "FR::FINISH-OK." << std::endl);
		return RET_FR_OK; // finish OK.

	default: break;
	}

	if (need_fct){ // read and check fifo control?
		LOGMSGS2 ( "FR::NEED_FCT, last: 0x"  << std::hex << last << std::dec << std::endl);

		lseek (dspdev, magic_data.probedatafifo, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
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
		LOGMSGS ( "FR::NEED_DATA" << std::endl);

		int database = (int)fifo.p_buffer_base;
		int dataleft = end_read;
		int position = 0;
		if (fifo.w_position > last_read_end){
//			database += last_read_end;
//			dataleft -= last_read_end;
		}
		for (; dataleft > 0; database += 0x4000, dataleft -= 0x4000, position += 0x4000){
			LOGMSGS ( "FR::NEED_DATA: B::0x" <<  std::hex << database <<  std::dec << std::endl);
			lseek (dspdev, database, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC); // ### CONSIDER NON_ATOMIC
			sr_read  (dspdev, &data[position], (dataleft >= 0x4000 ? 0x4000 : dataleft)<<1);
		}
		last_read_end = end_read;
			
		need_data = FR_NO;
	}

	if (need_hdr){ // we have enough data if control gets here!
		LOGMSGS ( "FR::NEED_HDR" << std::endl);

		// check for FIFO loop
		if (last > (EXTERN_PROBEDATAFIFO_LENGTH - EXTERN_PROBEDATA_MAX_LEFT)){
			LOGMSGS0 ( "FR:FIFO LOOP DETECTED -- FR::NEED_HDR ** Data @ " 
				   << "0x" << std::hex << last
				   << " -2 :[" << (*((DSP_LONG*)&data[last-2]))
				   << " " << (*((DSP_LONG*)&data[last]))
				   << " " << (*((DSP_LONG*)&data[last+2]))
				   << " " << (*((DSP_LONG*)&data[last+4]))
				   << " " << (*((DSP_LONG*)&data[last+6])) 
				   << "] : FIFO LOOP MARK " << std::dec << ( *((DSP_LONG*)&data[last]) == 0 ? "OK":"ERROR")
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

		DSPControlClass->add_probe_hdr (pv);
		

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
				LOGMSGS ( "END** FR::NEED_HDR: DATA_N ZERO -> END PROBE OK." << std::endl);
				DSPControlClass->probe_trigger_single_shot = 0; // if single shot, stop reading next
				number_channels = 0;
				data_index = 0;
				need_hdr = FR_YES;


				LOGMSGS ( "END** FR::FCT, w_position: 0x"  << std::hex << fifo.w_position << std::dec << std::endl);
				LOGMSGS ( "END** LastArdess: " << "0x" << std::hex << last << std::dec << std::endl);
				LOGMSGS ( "**FIFO::  HDR ALIGNMENT ASSUMING FOR MAPPING" << std::endl);
				LOGMSGS ( "**FIFO::  SRCS---- N------- t-------  X0-- Y0-- PHI-  XS-- YS-- ZS--  U--- SEC-" << std::endl);
				for (int adr=last-0x4000; adr < last+0x100; adr+=14){
					while (adr < 0) ++adr;
					LOGMSGS (  "0x" << std::setw(4) << std::hex << adr << "::"         // HDR
						   << " " << std::setw(8) << (*((DSP_LONG*)&data[adr]))    // srcs
						   << " " << std::setw(8) << (*((DSP_LONG*)&data[adr+2]))  // n
						   << " " << std::setw(8) << (*((DSP_LONG*)&data[adr+4]))  // t
						   << "  " << std::setw(4) << (*((DSP_INT*)&data[adr+6]))  // x0
						   << " " << std::setw(4) << (*((DSP_INT*)&data[adr+7]))   // y0
						   << " " << std::setw(4) << (*((DSP_INT*)&data[adr+8]))   // ph
						   << "  " << std::setw(4) << (*((DSP_INT*)&data[adr+9]))  // xs
						   << " " << std::setw(4) << (*((DSP_INT*)&data[adr+10]))  // ys
						   << " " << std::setw(4) << (*((DSP_INT*)&data[adr+11]))  // zs(5)
						   << "  " << std::setw(4) << (*((DSP_INT*)&data[adr+12])) // U(6)
						   << " " << std::setw(4) << (*((DSP_INT*)&data[adr+13]))  // sec
						   << std::dec << std::endl);
				}

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
	LOGMSGS ( "FR::DATA-cvt:"
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
					   << " -2 :[" << (*((DSP_LONG*)&data[last-2]))
					   << " " << (*((DSP_LONG*)&data[last]))
					   << " " << (*((DSP_LONG*)&data[last+2]))
					   << " " << (*((DSP_LONG*)&data[last+4]))
					   << " " << (*((DSP_LONG*)&data[last+6])) 
					   << "] : FIFO LOOP MARK " << std::dec << ( *((DSP_LONG*)&data[last]) == 0 ? "OK":"ERROR")
					   << std::endl);
				next_header -= last;
				last = -1; // compensate for ++last at of for(;;)!
				end_read = fifo.w_position;
			}
		}
	}

	LOGMSGS ( "FR:FIFO NEED FCT" << std::endl);
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
			  << " " << (*((DSP_LONG*)&data[adr]))
			  << " " << (*((DSP_LONG*)&data[adr+2]))
			  << " " << (*((DSP_LONG*)&data[adr+4]))
			  << " " << (*((DSP_LONG*)&data[adr+6]))
			  << std::dec << std::endl);
	}
	LOGMSGS0 ( "************** TRYING AUTO RECOVERY **************" << std::endl);
	LOGMSGS0 ( "***** -- STOP * RESET FIFO * START PROBE -- ******" << std::endl);
#endif
//	LOGMSGS0 ( "***** BAILOUT ****** TERM." << std::endl);

				LOGMSGS ( "END** FR::FCT, w_position: 0x"  << std::hex << fifo.w_position << std::dec << std::endl);
				LOGMSGS ( "END** LastArdess: " << "0x" << std::hex << last << std::dec << std::endl);
				LOGMSGS ( "**FIFO::  HDR ALIGNMENT ASSUMING FOR MAPPING" << std::endl);
				LOGMSGS ( "**FIFO::  SRCS---- N------- t-------  X0-- Y0-- PHI-  XS-- YS-- ZS--  U--- SEC-" << std::endl);
				for (int adr=last-0x4000; adr < last+0x100; adr+=14){
					while (adr < 0) ++adr;
					LOGMSGS (  "0x" << std::setw(4) << std::hex << adr << "::"         // HDR
						   << " " << std::setw(8) << (*((DSP_LONG*)&data[adr]))    // srcs
						   << " " << std::setw(8) << (*((DSP_LONG*)&data[adr+2]))  // n
						   << " " << std::setw(8) << (*((DSP_LONG*)&data[adr+4]))  // t
						   << "  " << std::setw(4) << (*((DSP_INT*)&data[adr+6]))  // x0
						   << " " << std::setw(4) << (*((DSP_INT*)&data[adr+7]))   // y0
						   << " " << std::setw(4) << (*((DSP_INT*)&data[adr+8]))   // ph
						   << "  " << std::setw(4) << (*((DSP_INT*)&data[adr+9]))  // xs
						   << " " << std::setw(4) << (*((DSP_INT*)&data[adr+10]))  // ys
						   << " " << std::setw(4) << (*((DSP_INT*)&data[adr+11]))  // zs(5)
						   << "  " << std::setw(4) << (*((DSP_INT*)&data[adr+12])) // U(6)
						   << " " << std::setw(4) << (*((DSP_INT*)&data[adr+13]))  // sec
						   << std::dec << std::endl);
				}

	DSPControlClass->dump_probe_hdr (); // TESTING
				

	// *****	exit (0);

	// STOP PROBE
	// reset positions now to prevent reading old/last dataset before DSP starts init/putting data
	DSP_INT start_stop[2] = { 0, 1 };
	lseek (dspdev, magic_data.probe, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_write (dspdev, &start_stop, 2*sizeof(DSP_INT));

	// RESET FIFO
	// reset positions now to prevent reading old/last dataset before DSP starts init/putting data
	fifo.r_position = 0;
	fifo.w_position = 0;
	check_and_swap (fifo.r_position);
	check_and_swap (fifo.w_position);
	lseek (dspdev, magic_data.probedatafifo, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_write (dspdev, &fifo, 2*sizeof(DSP_INT));

	// START PROBE
	// reset positions now to prevent reading old/last dataset before DSP starts init/putting data
	start_stop[0] = 1;
	start_stop[1] = 0;
	lseek (dspdev, magic_data.probe, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_write (dspdev, &start_stop, 2*sizeof(DSP_INT));

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
const gchar* sranger_mk2_hwi_dev::get_info(){
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
			"Magic....... : %04X\n"
			"Version..... : %04X\n"
			"Date........ : %04X%04X\n"
			"*-- DSP Control Struct Locations --*\n"
			"statemachine : %04x\n"
			"feedback.... : %04x\n"
			"feedback_mix.: %04x\n"
			"analog...... : %04x\n"
			"move........ : %04x\n"
			"scan........ : %04x\n"
			"probe....... : %04x\n"
			"autoapproach : %04x\n"
			"datafifo.... : %04x\n"
			"probedatafifo: %04x\n"
			"------------------------------------\n"
			"%s",
			magic_data.magic,
			magic_data.version,
			magic_data.year,
			magic_data.mmdd,
			magic_data.statemachine,
			magic_data.feedback,
			magic_data.feedback_mixer,
			magic_data.analog,
			magic_data.move,
			magic_data.scan,
			magic_data.probe,
			magic_data.autoapproach,
			magic_data.datafifo,
			magic_data.probedatafifo,
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

double sranger_mk2_hwi_dev::GetUserParam (gint n, gchar *id){
	return DSPControlClass->GetUserParam (n, id);
}

gint sranger_mk2_hwi_dev::SetUserParam (gint n, gchar *id, double value){
	return DSPControlClass->SetUserParam (n, id, value);
}


// ======================================== ========================================





#define CONV_16_C(X) int_2_sranger_int (X)
#define CONV_32_C(X) long_2_sranger_long (X)

#define CONV_16(X) X = int_2_sranger_int (X)
#define CONV_32(X) X = long_2_sranger_long (X)



void sranger_mk2_hwi_dev::read_dsp_state (gint32 &mode){
	lseek (dsp, magic_data.statemachine, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_read (dsp, &dsp_state, sizeof(dsp_state)); 

	CONV_16 (dsp_state.mode);
	CONV_32 (dsp_state.DataProcessTime);
	CONV_32 (dsp_state.DataProcessReentryTime);
	CONV_32 (dsp_state.DataProcessReentryPeak);
	CONV_32 (dsp_state.IdleTime_Peak);

	mode = dsp_state.mode;
}

void sranger_mk2_hwi_dev::write_dsp_state (gint32 mode){
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
void sranger_mk2_hwi_dev::conv_dsp_feedback (){
	CONV_16 (dsp_feedback.cp);
	CONV_16 (dsp_feedback.ci);
	CONV_16 (dsp_feedback.setpoint);
	CONV_16 (dsp_feedback.ca_q15);
	CONV_32 (dsp_feedback.cb_Ic);
	CONV_16 (dsp_feedback.I_cross);
	CONV_16 (dsp_feedback.I_offset);
	CONV_16 (dsp_feedback.q_factor15);

	for (int i=0; i<4; ++i){
		CONV_16 (dsp_feedback_mixer.level[i]);
		CONV_16 (dsp_feedback_mixer.gain[i]);
		CONV_16 (dsp_feedback_mixer.mode[i]);
		CONV_16 (dsp_feedback_mixer.setpoint[i]);
	}
        CONV_16 (dsp_feedback_mixer.Z_setpoint);
}

double sranger_mk2_hwi_dev::read_dsp_feedback (const gchar *property, int index){
	lseek (dsp, magic_data.feedback, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_read (dsp, &dsp_feedback, (MAX_WRITE_SPM_PI_FEEDBACK+2)<<1);  // read back "f0-now" (q_factor_15)

	lseek (dsp, magic_data.feedback_mixer, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_read (dsp, &dsp_feedback_mixer, MAX_WRITE_FEEDBACK_MIXER<<1);

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

        
        return 0.;
}

void sranger_mk2_hwi_dev::write_dsp_feedback (
		double set_point[4], double set_point_factor[4], double gain[4], double level[4], int transform_mode[4],
		double IIR_I_crossover, double IIR_f0_max[4], double IIR_f0_min, double LOG_I_offset, int IIR_flag,
		double setpoint_zpos, double z_servo[3], double m_servo[3], double pllref){

	for (int i=0; i<4; ++i){
		if (i==0){
			dsp_feedback_mixer.setpoint[i] = (int)(round(gapp->xsm->Inst->VoltIn2Dig (gapp->xsm->Inst->nAmpere2V (set_point[i])))); // Q23
                }else if (i==1){
                        dsp_feedback_mixer.setpoint[i] = (int)round(256.*round(gapp->xsm->Inst->VoltIn2Dig (gapp->xsm->Inst->dHertz2V (set_point[i])))); // Q23
                }else
			dsp_feedback_mixer.setpoint[i] = (int)(round(gapp->xsm->Inst->VoltIn2Dig (set_point[i]))); // Q23
                // dsp_feedback_mixer.setpoint[i] = (int)(round(gapp->xsm->Inst->VoltIn2Dig (set_point_factor[i]*set_point[i]))); // Q23
		dsp_feedback_mixer.level[i]    = (int)(round(gapp->xsm->Inst->VoltIn2Dig (level[i])));
		//dsp_feedback_mixer.level[i]    = (int)(round(gapp->xsm->Inst->VoltIn2Dig (set_point_factor[i]*level[i])));
		dsp_feedback_mixer.gain[i]     = float_2_sranger_q15 (gain[i]);
		dsp_feedback_mixer.mode[i]     = transform_mode[i];
	}
	// feedback settings
	// Scale Regler Consts. with 1/VZ and convert to SR-Q15
	// MK3 compatibility scaling user input CP,CI now x100
	dsp_feedback.cp = float_2_sranger_q15 (0.01 * z_servo[SERVO_CP] / sranger_mk2_hwi_pi.app->xsm->Inst->VZ ());
	dsp_feedback.ci = float_2_sranger_q15 (0.01 * z_servo[SERVO_CI] / sranger_mk2_hwi_pi.app->xsm->Inst->VZ ());

	// IIR self adaptive filter parameters
	double ca,cb;
	double Ic = gapp->xsm->Inst->VoltIn2Dig (gapp->xsm->Inst->nAmpere2V (1e-3*IIR_I_crossover)); // given in pA
	dsp_feedback.ca_q15   = float_2_sranger_q15 (ca=exp(-2.*M_PI*IIR_f0_max[0]/75000.));
	dsp_feedback.cb_Ic    = (DSP_LONG) (32767. * (cb = IIR_f0_min/IIR_f0_max[0] * Ic));
	dsp_feedback.I_cross  = (int)Ic; 
	dsp_feedback.I_offset = 1+(int)(gapp->xsm->Inst->VoltIn2Dig (gapp->xsm->Inst->nAmpere2V (1e-3*LOG_I_offset))); // given in pA

        dsp_feedback_mixer.Z_setpoint   = (int)(round(gapp->xsm->Inst->ZA2Dig (setpoint_zpos))); // new ZPos reference for FUZZY-LOG CONST HEIGHT 

	if (!IIR_flag)
		dsp_feedback.I_cross = 0; // disable IIR!

#if 0
	if (!IIR_flag)
		std::cout << "IIR: off" << std::endl;
	else {
		std::cout << "IIR: ca'    Q15= " << dsp_feedback.ca_q15 << " := " << ca << std::endl;
		std::cout << "IIR: cb_Ic' Q31= " << dsp_feedback.cb_Ic  << " := " << cb << std::endl;
		std::cout << "IIR: Ic        = " << dsp_feedback.I_cross  << std::endl;
		std::cout << "IIR: Io        = " << dsp_feedback.I_offset << std::endl;
	}
#endif

	// from PC to DSP
	conv_dsp_feedback ();

	lseek (dsp, magic_data.feedback, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_write (dsp, &dsp_feedback, MAX_WRITE_SPM_PI_FEEDBACK<<1); 

	lseek (dsp, magic_data.feedback_mixer, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_write (dsp, &dsp_feedback_mixer, MAX_WRITE_FEEDBACK_MIXER<<1);

	// from DSP to PC
	conv_dsp_feedback ();
}

void sranger_mk2_hwi_dev::recalculate_dsp_scan_speed_parameters () {
	double tmp;
	
	DSPControlClass->recalculate_dsp_scan_speed_parameters (tmp_dnx, tmp_dny, tmp_fs_dx, tmp_fs_dy, tmp_nx_pre, tmp_fast_return);
	
	PI_DEBUG_GP (DBG_L1, "*** recalculated DSP scan parameters [MK2] ***\n");
	PI_DEBUG_GP (DBG_L1, "sizeof dsp_scan   %d B\n", (int) sizeof (dsp_scan));
	PI_DEBUG_GP (DBG_L1, "sizeof scan.fs_dx %d B\n", (int) sizeof (dsp_scan.fs_dx));

	dsp_scan.fs_dx  = (DSP_LONG)tmp_fs_dx;
	dsp_scan.fs_dy  = (DSP_LONG)tmp_fs_dy;
	dsp_scan.dnx    = (DSP_INT)tmp_dnx;
	dsp_scan.dny    = (DSP_INT)tmp_dny;
	dsp_scan.nx_pre = (DSP_INT)tmp_nx_pre;
	dsp_scan.fast_return = (DSP_INT)tmp_fast_return;
	tmp = 1000. + 3. * ( fabs ((double)dsp_scan.fs_dx*(double)dsp_scan.fm_dz0_xy_vec[i_X]) + fabs((double)dsp_scan.fs_dy*(double)dsp_scan.fm_dz0_xy_vec[i_Y]) )/2147483648.; // =1<<31
	PI_DEBUG_GP (DBG_L1, "Z-Slope-Max 3x (real): %g \n", tmp);
	if (tmp > 67108864.)
		tmp = 67108864.; // 1<<26
	PI_DEBUG_GP (DBG_L1, "Z-Slope-Max (real, lim): %g \n", tmp);
	dsp_scan.z_slope_max = (DSP_LONG)(tmp);

#if 0
	if (tmp_fast_return == -1){ // -1: indicates fast scan mode with X sin actuation
#define CPN(N) ((double)(1LL<<(N))-1.)
		double deltasRe, deltasIm;
		// calculate Sine generator parameter
		// -- here 2147483647 * cos (2pi/N), N=n*dnx*2+dny   **** cos (2.*M_PI*freq[0]/150000.))
		deltasRe = round (CPN(31) * cos (2.*M_PI/((double)dsp_scan.dnx * (double)Nx * 2. + (double)dsp_scan.dny)));
		deltasIm = round (CPN(31) * sin (2.*M_PI/((double)dsp_scan.dnx * (double)Nx * 2. + (double)dsp_scan.dny)));
		dsp_scan.Zoff_2nd_xp = (DSP_LONG)deltasRe; // shared variables, no 2nd line mode in fast scan!
		dsp_scan.Zoff_2nd_xm = (DSP_LONG)deltasIm;
	}
#endif

	
	PI_DEBUG_GP (DBG_L1, "x,y slope....** %d, %d \n", dsp_scan.fm_dz0_xy_vec[i_X], dsp_scan.fm_dz0_xy_vec[i_Y]);
	PI_DEBUG_GP (DBG_L1, "z_slope_max..** %d \n", dsp_scan.z_slope_max);
	PI_DEBUG_GP (DBG_L1, "FSdX.........** %d \n", dsp_scan.fs_dx);
	PI_DEBUG_GP (DBG_L1, "FSdY.........** %d \n", dsp_scan.fs_dy);
	PI_DEBUG_GP (DBG_L1, "DNX..........** %d \n", dsp_scan.dnx);
	PI_DEBUG_GP (DBG_L1, "DNY..........** %d \n", dsp_scan.dny);
	PI_DEBUG_GP (DBG_L1, "NXPre........** %d \n", dsp_scan.nx_pre);
}

void sranger_mk2_hwi_dev::recalculate_dsp_scan_slope_parameters () {
	double tmp;
	double swx = (double)dsp_scan.fs_dx * (double)dsp_scan.dnx * (double)dsp_scan.nx / 4294967296.; // =1<<32
	double swy = (double)dsp_scan.fs_dy * (double)dsp_scan.dny * (double)dsp_scan.ny / 4294967296.; // =1<<32
	DSPControlClass->recalculate_dsp_scan_slope_parameters (dsp_scan.fs_dx, dsp_scan.fs_dy,
								dsp_scan.fm_dz0_xy_vec[i_X], dsp_scan.fm_dz0_xy_vec[i_Y],
								swx, swy);
	PI_DEBUG_GP (DBG_L1, "*** recalculated DSP slope parameters ***\n");

	PI_DEBUG_GP (DBG_L1, "FM_dz0x......** %d\n", dsp_scan.fm_dz0_xy_vec[i_X]);
	PI_DEBUG_GP (DBG_L1, "FM_dz0y......** %d\n", dsp_scan.fm_dz0_xy_vec[i_Y]);

	tmp = 1000. + 3. * ( fabs ((double)dsp_scan.fs_dx*(double)dsp_scan.fm_dz0_xy_vec[i_X]) + fabs((double)dsp_scan.fs_dy*(double)dsp_scan.fm_dz0_xy_vec[i_Y]) )/2147483648.; // =1<<31
	PI_DEBUG_GP (DBG_L1, "Z-Slope-Max 3x (real): %g \n", tmp);
	if (tmp > 67108864.)
		tmp = 67108864.; // 1<<26
	PI_DEBUG_GP (DBG_L1, "Z-Slope-Max (real, lim): %g \n", tmp);
	dsp_scan.z_slope_max = (DSP_LONG)(tmp);
}

void sranger_mk2_hwi_dev::conv_dsp_analog (){
	CONV_16 (dsp_analog.out[ANALOG_BIAS]);
	CONV_16 (dsp_analog.out[ANALOG_MOTOR]);
}

void sranger_mk2_hwi_dev::read_dsp_analog (){
	// read all analog
	lseek (dsp, magic_data.analog, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_read  (dsp, &dsp_analog, sizeof (dsp_analog)); 

	// from DSP to PC
	conv_dsp_analog ();
}

void sranger_mk2_hwi_dev::write_dsp_analog (double bias[4], double motor){
	read_dsp_analog ();

	dsp_analog.out[ANALOG_BIAS]  = (int)(gapp->xsm->Inst->VoltOut2Dig (gapp->xsm->Inst->BiasV2Vabs (bias[0])));
	dsp_analog.out[ANALOG_MOTOR] = (int)(gapp->xsm->Inst->VoltOut2Dig (gapp->xsm->Inst->BiasV2Vabs (motor)));

	// only "bias" and "motor" is touched here!

	// from PC to DSP
	conv_dsp_analog ();

	lseek (dsp, magic_data.analog, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_write (dsp, dsp_analog.out, MAX_WRITE_ANALOG<<1); 

	// from DSP to PC
	conv_dsp_analog ();

        // update bias section in dsp_scan
        gint32 x;
        read_dsp_scan (x);
	dsp_scan.bias_section[0]  = (int)(gapp->xsm->Inst->VoltOut2Dig (gapp->xsm->Inst->BiasV2Vabs (bias[0])));
	dsp_scan.bias_section[1]  = (int)(gapp->xsm->Inst->VoltOut2Dig (gapp->xsm->Inst->BiasV2Vabs (bias[1])));
	dsp_scan.bias_section[2]  = (int)(gapp->xsm->Inst->VoltOut2Dig (gapp->xsm->Inst->BiasV2Vabs (bias[2])));
	dsp_scan.bias_section[3]  = (int)(gapp->xsm->Inst->VoltOut2Dig (gapp->xsm->Inst->BiasV2Vabs (bias[3])));
        write_dsp_scan ();
}

void sranger_mk2_hwi_dev::conv_dsp_scan (){
	CONV_32 (dsp_scan.rotm[0]);
	CONV_32 (dsp_scan.rotm[1]);
	CONV_32 (dsp_scan.rotm[2]);
	CONV_32 (dsp_scan.rotm[3]);
	CONV_32 (dsp_scan.z_slope_max);
	CONV_16 (dsp_scan.fast_return);
	CONV_16 (dsp_scan.fast_return_2nd);
	CONV_16 (dsp_scan.slow_down_factor);
	CONV_16 (dsp_scan.slow_down_factor_2nd);
	CONV_16 (dsp_scan.bias_section[0]);
	CONV_16 (dsp_scan.bias_section[1]);
	CONV_16 (dsp_scan.bias_section[2]);
	CONV_16 (dsp_scan.bias_section[3]);
	CONV_16 (dsp_scan.nx_pre);
	CONV_16 (dsp_scan.dnx_probe);
	CONV_16 (dsp_scan.raster_a);
	CONV_16 (dsp_scan.raster_b);
	CONV_32 (dsp_scan.srcs_xp);
	CONV_32 (dsp_scan.srcs_xm);
	CONV_32 (dsp_scan.srcs_2nd_xp);
	CONV_32 (dsp_scan.srcs_2nd_xm);
	CONV_16 (dsp_scan.nx);
	CONV_16 (dsp_scan.ny);
	CONV_32 (dsp_scan.fs_dx);
	CONV_32 (dsp_scan.fs_dy);
	CONV_32 (dsp_scan.num_steps_move_xy);
	CONV_32 (dsp_scan.fm_dx);
	CONV_32 (dsp_scan.fm_dy); 
	CONV_32 (dsp_scan.fm_dzxy);
	CONV_32 (dsp_scan.fm_dz0_xy_vec[i_X]);
	CONV_32 (dsp_scan.fm_dz0_xy_vec[i_Y]);
	CONV_16 (dsp_scan.dnx);
	CONV_16 (dsp_scan.dny);
	CONV_16 (dsp_scan.Zoff_2nd_xp);
	CONV_16 (dsp_scan.Zoff_2nd_xm);
	CONV_32 (dsp_scan.xyz_vec[i_X]);
	CONV_32 (dsp_scan.xyz_vec[i_Y]);
	CONV_32 (dsp_scan.cfs_dx);
	CONV_32 (dsp_scan.cfs_dy);
// RO:
	CONV_16 (dsp_scan.sstate);
	CONV_16 (dsp_scan.pflg);
}

void sranger_mk2_hwi_dev::read_dsp_scan (gint32 &pflg){
	lseek (dsp, magic_data.scan, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_read  (dsp, &dsp_scan, sizeof (dsp_scan));

	// from DSP to PC
	conv_dsp_scan ();

	pflg = dsp_scan.pflg;
}

void sranger_mk2_hwi_dev::write_dsp_scan (){
	// from PC to DSP
	conv_dsp_scan ();

	// adjust ScanParams/Speed...
	lseek (dsp, magic_data.scan, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_write (dsp, &dsp_scan, MAX_WRITE_SCAN<<1);

	// from DSP to PC
	conv_dsp_scan ();
}


void sranger_mk2_hwi_dev::conv_dsp_probe (){
	CONV_16 (dsp_probe.start);
	CONV_16 (dsp_probe.LIM_up);
	CONV_16 (dsp_probe.LIM_dn);
	CONV_16 (dsp_probe.AC_amp);
	CONV_16 (dsp_probe.AC_frq);
	CONV_16 (dsp_probe.AC_phaseA);
	CONV_16 (dsp_probe.AC_phaseB);
	CONV_16 (dsp_probe.AC_nAve);
	CONV_32 (dsp_probe.Zpos);
//	CONV_16 (dsp_probe.);
//	CONV_32 (dsp_probe.);
	CONV_16 (dsp_probe.vector_head);

}
	
void sranger_mk2_hwi_dev::read_dsp_lockin (double AC_amp[4], double &AC_frq, double &AC_phaseA, double &AC_phaseB, gint32 &AC_lockin_avg_cycels){
	lseek (dsp, magic_data.probe, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_read (dsp, &dsp_probe, sizeof (dsp_probe)); 

	// from DSP to PC
	conv_dsp_probe ();

	// update, reconvert
	AC_amp[0] = gapp->xsm->Inst->Dig2VoltOut ((double)dsp_probe.AC_amp) * gapp->xsm->Inst->BiasGainV2V ();
	AC_frq = dsp_probe.AC_frq; // tmp fix, DSP assumes still 22000
	if (AC_frq < 32)
 		AC_frq = 1. + 22000.*AC_frq/128.;

	AC_frq *= 75./22.; // tmp fix, DSP assumes still 22000

	AC_phaseA = (double)dsp_probe.AC_phaseA/16.;
	AC_phaseB = (double)dsp_probe.AC_phaseB/16.;
	AC_lockin_avg_cycels = dsp_probe.AC_nAve;
}

int sranger_mk2_hwi_dev::dsp_lockin_state(int set){
	lseek (dsp, magic_data.probe, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_read (dsp, &dsp_probe, sizeof (dsp_probe));
	dsp_probe.start = 0;
	dsp_probe.stop  = 0;
	switch (set){
	case 0:	dsp_probe.stop = PROBE_RUN_LOCKIN_FREE;
		CONV_16 (dsp_probe.start);
		CONV_16 (dsp_probe.stop);
		lseek (dsp, magic_data.probe, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
		sr_write (dsp, &dsp_probe,  MAX_WRITE_PROBE<<1); 
		return 0;
		break;
	case 1:	dsp_probe.start = PROBE_RUN_LOCKIN_FREE;
		CONV_16 (dsp_probe.start);
		CONV_16 (dsp_probe.stop);
		lseek (dsp, magic_data.probe, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
		sr_write (dsp, &dsp_probe,  MAX_WRITE_PROBE<<1); 
		return 0;
		break;
	case -1:
		CONV_16 (dsp_probe.state);
		return dsp_probe.state == PROBE_RUN_LOCKIN_FREE ? 1:0;
		break;
	}
	return -1;
}

void sranger_mk2_hwi_dev::write_dsp_lockin_probe_final (double AC_amp[4], double &AC_frq, double AC_phaseA, double AC_phaseB, gint32 AC_lockin_avg_cycels, double VP_lim_val[2], double noise_amp, int start) {
	// update all from DSP
	lseek (dsp, magic_data.probe, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_read (dsp, &dsp_probe, sizeof (dsp_probe));

	// from DSP to PC
	conv_dsp_probe ();
	
	// update with changed user parameters
	dsp_probe.start = start ? 1:0;
	dsp_probe.LIM_up = VOLT2AIC (VP_lim_val[0]);
	dsp_probe.LIM_dn = VOLT2AIC (VP_lim_val[1]);
	dsp_probe.AC_amp = DVOLT2AIC (AC_amp[0]);
	dsp_probe.AC_frq = (int) (AC_frq * 22./75.); // DSP still assumes 22000
	dsp_probe.AC_phaseA = (int)round(AC_phaseA*16.);
	dsp_probe.AC_phaseB = (int)round(AC_phaseB*16.);
	dsp_probe.AC_nAve = AC_lockin_avg_cycels;

	// from PC to DSP
	conv_dsp_probe ();
	lseek (dsp, magic_data.probe, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_write (dsp, &dsp_probe,  MAX_WRITE_PROBE<<1); 
	// from DSP to PC
	conv_dsp_probe ();

	const int AC_TAB_INC = 1;
	int AC_tab_inc=0;

	if (AC_frq > 1000)
		AC_tab_inc = 8*AC_TAB_INC;     // LockIn-Ref on 1375Hz
	else if (AC_frq > 500)
		AC_tab_inc = 4*AC_TAB_INC;     // LockIn-Ref on 687.5Hz
	else if (AC_frq > 250)
		AC_tab_inc = 2*AC_TAB_INC;     // LockIn-Ref on 343.75Hz
	else AC_tab_inc = 1*AC_TAB_INC;     // LockIn-Ref on 171.875Hz
	
	AC_frq = 75000./128.*AC_tab_inc;
}


void sranger_mk2_hwi_dev::conv_dsp_vector (){
	CONV_32 (dsp_vector.n);
	CONV_32 (dsp_vector.dnx);  
	CONV_32 (dsp_vector.srcs);
	CONV_32 (dsp_vector.options);
	CONV_16 (dsp_vector.ptr_fb);
	CONV_16 (dsp_vector.repetitions);
	CONV_16 (dsp_vector.i);
	CONV_16 (dsp_vector.j);
	CONV_16 (dsp_vector.ptr_next);
	CONV_16 (dsp_vector.ptr_final);
	CONV_32 (dsp_vector.f_du);
	CONV_32 (dsp_vector.f_dx);
	CONV_32 (dsp_vector.f_dy);
	CONV_32 (dsp_vector.f_dz);
	CONV_32 (dsp_vector.f_dx0);
	CONV_32 (dsp_vector.f_dy0);
	CONV_32 (dsp_vector.f_dphi);
}

// PZ may need t oadjust...
#define EXTERN_PROBE_VECTOR_HEAD_DEFAULT 0x4100

void sranger_mk2_hwi_dev::write_dsp_vector (int index, PROBE_VECTOR_GENERIC *__dsp_vector){
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
	  if (dsp_vector.dnx < 0 || dsp_vector.dnx > 32767 || dsp_vector.n <= 0 || dsp_vector.n > 2147483647){
                  gchar *msg = g_strdup_printf ("Probe Vector [MK2] [pc%02d] not acceptable:\n"
                                                "n   = %6d   [1..2147483647]\n"
                                                "dnx = %6d   [0..32767]\n"
                                                "Auto Limiting.\n"
                                                "Hint: adjust slope/speed/points/time.",
                                                index, dsp_vector.n, dsp_vector.dnx);
                  gapp->message (msg);
                  g_free (msg);
                  if (dsp_vector.dnx < 0) dsp_vector.dnx = 0;
                  if (dsp_vector.dnx > 32767) dsp_vector.dnx = 32767;
                  if (dsp_vector.n <= 0) dsp_vector.n = 1;
                  if (dsp_vector.n > 2147483647) dsp_vector.n = 2147483647;
	  }

	// setup VPC essentials
	dsp_vector.i = dsp_vector.repetitions; // preload repetitions counter now! (if now already done)
	dsp_vector.j = 0; // not yet used -- XXXX

	// from PC to to DSP format
        conv_dsp_vector();
	lseek (dsp, dsp_probe.vector_head + index*SIZE_OF_PROBE_VECTOR, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_write (dsp, &dsp_vector, SIZE_OF_PROBE_VECTOR<<1);

	// from DSP to PC
        conv_dsp_vector();
}

void sranger_mk2_hwi_dev::read_dsp_vector (int index, PROBE_VECTOR_GENERIC *__dsp_vector){

	// update all from DSP
	lseek (dsp, magic_data.probe, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_read (dsp, &dsp_probe, sizeof (dsp_probe)); 
	// from DSP to PC
	conv_dsp_probe ();

	if (dsp_probe.vector_head < EXTERN_PROBE_VECTOR_HEAD_DEFAULT || index > 40 || index < 0){
		sranger_mk2_hwi_pi.app->message("Error reading Probe Vector:\n Bad vector address, aborting request.");
		return;
	}

	lseek (dsp, dsp_probe.vector_head + index*SIZE_OF_PROBE_VECTOR, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
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

void sranger_mk2_hwi_dev::write_dsp_abort_probe () {
	PROBE dsp_probe;
	dsp_probe.start = 0;
	dsp_probe.stop  = 1;
        CONV_16 (dsp_probe.start);
        CONV_16 (dsp_probe.stop);
        lseek (dsp, magic_data.probe, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
        sr_write (dsp, &dsp_probe,  (2)<<1); 
        usleep ((useconds_t) (5000) ); // give some time to abort
}


void sranger_mk2_hwi_dev::write_dsp_reset (){
	int ret;
	if ((ret=ioctl(dsp, SRANGER_MK2_IOCTL_ASSERT_DSP_RESET, NULL)) < 0){
		sranger_mk2_hwi_pi.app->message("IOCTL Error: DSP RESET ASSERT failed.");
	}

	usleep(500000);
			
	if ((ret=ioctl(dsp, SRANGER_MK2_IOCTL_RELEASE_DSP_RESET, NULL)) < 0){
		sranger_mk2_hwi_pi.app->message("IOCTL Error: DSP RESET RELEASE failed.");
	}			

	usleep(500000);

	sranger_mk2_hwi_pi.app->message("SR-MK2 IOCTL: DSP RESET CYCLE done.");
}

