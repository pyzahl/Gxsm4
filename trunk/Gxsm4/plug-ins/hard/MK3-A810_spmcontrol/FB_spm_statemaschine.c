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

/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

#include "g_intrinsics.h"
#include "FB_spm_statemaschine.h"
#include "FB_spm_analog.h"
#include "dataprocess.h"
#include "ReadWrite_GPIO.h"
#ifdef USE_PLL_API
#include "PAC_pll.h"
#endif

#include "mcbsp_support.h"

#include "FB_spm_task_names.h"

extern SPM_STATEMACHINE state;
extern FEEDBACK_MIXER   feedback_mixer;
extern SERVO            z_servo;
extern SERVO            m_servo;
extern ANALOG_VALUES    analog;
extern MOVE_OFFSET      move;
extern AREA_SCAN        scan;
extern PROBE            probe;
extern AUTOAPPROACH     autoapp;
extern CR_OUT_PULSE     CR_out_pulse;
extern CR_GENERIC_IO    CR_generic_io;
extern SIGNAL_MONITOR   sig_mon;
extern A810_CONFIG      a810_config;
extern PLL_LOOKUP       PLL_lookup;
extern DATA_SYNC_IO     data_sync_io;

extern int PRB_AIC_data_sum[9];

extern int  GPIO_subsample; // from probe module

extern void copy32(int *restrict x, int *restrict r, unsigned short nx);

/*
 *	DSP idle loop, runs for ever, returns never !!!!
 *  ============================================================
 *	Main of the DSP State Maschine
 *      - State Status "STMMode", heartbeat
 *      - manage process commands via state, 
 *        this may change the state
 *      - enable/disable of all tasks
 */

DSP_INT32 setpoint_old = 0;
DSP_INT32 mix_setpoint_old[4] = {0,0,0,0};

int AIC_stop_mode = 0;
int sleepcount = 0;

extern SPM_MAGIC_DATA_LOCATIONS magic;

extern int bz_push_area_scan_data_out (void);

//#define MANUALCONTROL
#ifdef MANUALCONTROL
#define STOP_RT_TASK(N)         if(!state.id_task_control[3].process_flag&0x40) state.dp_task_control[N].process_flag &= 0xffff0000
#define START_RT_TASK(N)        if(!state.id_task_control[3].process_flag&0x40) state.dp_task_control[N].process_flag |= 0x10
#define START_RT_TASK_ALWAYS(N) if(!state.id_task_control[3].process_flag&0x40) state.dp_task_control[N].process_flag |= 0x10
#define START_RT_TASK_EVEN(N)   if(!state.id_task_control[3].process_flag&0x40) state.dp_task_control[N].process_flag |= 0x20
#define START_RT_TASK_ODD(N)    if(!state.id_task_control[3].process_flag&0x40) state.dp_task_control[N].process_flag |= 0x40
#else
#define STOP_RT_TASK(N)         state.dp_task_control[N].process_flag &= 0xffff0000
#define START_RT_TASK(N)        state.dp_task_control[N].process_flag |= 0x10
#define START_RT_TASK_ALWAYS(N) state.dp_task_control[N].process_flag |= 0x10
#define START_RT_TASK_EVEN(N)   state.dp_task_control[N].process_flag |= 0x20
#define START_RT_TASK_ODD(N)    state.dp_task_control[N].process_flag |= 0x40
#endif

#define STOP_ID_TASK(N)         state.id_task_control[N].process_flag = 0x00
#define START_ID_TASK(N)        state.id_task_control[N].process_flag |= 0x10
#define START_ID_TASK_TMR(N)    state.id_task_control[N].process_flag |= 0x20
#define START_ID_TASK_CLK(N)    state.id_task_control[N].process_flag |= 0x40



#define BIAS_ADJUST_STEP (4*(1<<16))
#define Z_ADJUST_STEP    (0x200)

/* #define AIC_OUT(N) iobuf.mout[N] */
/* smoothly adjust bias - make sure |analog_bias| < 32766-BIAS_ADJUST_STEP !!  */
#if 0
inline void run_bias_adjust (){
	if (analog.bias[scan.section] - BIAS_ADJUST_STEP > analog.bias_adjust){
		analog.bias_adjust += BIAS_ADJUST_STEP;
	}else{	
		if (analog.bias[scan.section] + BIAS_ADJUST_STEP < analog.bias_adjust)
			analog.bias_adjust -= BIAS_ADJUST_STEP;
		else	
			analog.bias_adjust = analog.bias[scan.section];
	}
}
#endif

/* smoothly brings Zpos back to zero in case VP left it non zero at finish */
inline void run_Zpos_adjust (){
	if (probe.Zpos > Z_ADJUST_STEP)
		probe.Zpos -= Z_ADJUST_STEP;
	else
		if (probe.Zpos < -Z_ADJUST_STEP)
			probe.Zpos += Z_ADJUST_STEP;
		else
			probe.Zpos = 0;
}


void configure_DSP_GP_PINS();

#pragma CODE_SECTION(idle_task_001, ".text:slow")
int idle_task_001(void){
        return bz_push_area_scan_data_out ();
}

#ifdef MIN_PAC_BUILD
extern DSP_INT32  s_xymult;
extern DSP_INT32 xy_vec[4];
extern DSP_INT32 result_vec[4];
#endif

#pragma CODE_SECTION(idle_task_002, ".text:slow")
int idle_task_002(void){
#ifdef MIN_PAC_BUILD
        DSP_INT32  d_tmp;
#endif
        // ============================================================
        // PROCESS MODULE: OFFSET MOVE
        // ============================================================
        /* Offset Move task ? */
        if (move.pflg){
                run_offset_move ();
                return 1;
        }

#ifdef MIN_PAC_BUILD // AREA_SCAN_TASK non RT task

        // ============================================================
        // PROCESS MODULE: AREA SCAN
        // ============================================================
        /* Area Scan task - normal mode ?
         * the feedback task needs to be enabled to see the effect
         * --> can set CI/CP to small values to "contineously" disable it!
         */
        if (scan.pflg & (AREA_SCAN_RUN | AREA_SCAN_MOVE_TIP))
                if (!probe.pflg || probe.start) // pause scan if raster_b!=0 and probe is going.
                        run_area_scan ();

        // ============================================================
        // PROCESS MODULE: FAST SCAN
        // ============================================================

        /* run FAST AREA SCAN (sinusodial) ? */
        if (scan.pflg & AREA_SCAN_RUN_FAST)
                run_area_scan_fast ();
                
        // ============================================================
        // do expensive 32bit precision (tmp64/40) rotation in
        // EVEN cycle
        // ============================================================
                
        // NOW OUTPUT HR SIGNALS ON XYZ-Offset and XYZ-Scan -- do not touch Bias OUT(6) and Motor OUT(7) here -- handled directly previously.
        // note: OUT(0-5) get overridden below by coarse/mover actions if requeste!!!

        /* HR sigma-delta data processing (if enabled) -- turn off via adjusting sigma_delta_hr_mask to all 0 */

#ifdef MIN_PAC_BUILD_NO_ROTATION
        scan.xy_r_vec[i_X] = scan.xyz_vec[i_X];
        scan.xy_r_vec[i_Y] = scan.xyz_vec[i_Y];
#else
        // do scan coordinate rotation transformation:
        if ( !(state.mode & MD_XYSROT)){
                xy_vec[2] = xy_vec[0] = scan.xyz_vec[i_X];
                xy_vec[3] = xy_vec[1] = scan.xyz_vec[i_Y];
                mul32 (xy_vec, scan.rotmatrix, result_vec, 4);
                scan.xy_r_vec[i_X] = _SADD32 (result_vec[0], result_vec[1]);
                scan.xy_r_vec[i_Y] = _SADD32 (result_vec[2], result_vec[3]);
        } else {
                scan.xy_r_vec[i_X] = scan.xyz_vec[i_X];
                scan.xy_r_vec[i_Y] = scan.xyz_vec[i_Y];
        }
#endif
        // XY-Offset and XY-Scan output -- calculates Scan XY output and added offset as configured
        // default: HR_OUT[3,4] = scan.xy_r_vec + move.xyz_vec
        //** done below in one shot loop **
        //**	compute_analog_out (3, &analog.out[3]);
        //**	compute_analog_out (4, &analog.out[4]);

        // PROCESS MODULE: SLOPE-COMPENSATION
        // ==================================================
        // Z-Offset -- slope compensation output
        //---- slope add X*mx + Y*my
        // limit dz add from xy-mult to say 10x scan.fm_dz0x+y, feedback like adjust if "diff" to far off from sudden slope change 
        //      zpos_xymult = move.ZPos + scan.XposR * scan.fm_dz0x +  scan.YposR * scan.fm_dz0y ;
        // make sure a smooth adjust -- if slope parameters get changed, need to prevent a jump.

        mul32 (scan.xy_r_vec, scan.fm_dz0_xy_vec, result_vec, 2);
        s_xymult = _SADD32 (result_vec[i_X], result_vec[i_Y]);
        
        d_tmp = _SSUB32 (s_xymult, scan.z_offset_xyslope);
        if (d_tmp > scan.z_slope_max) // limit up
                scan.z_offset_xyslope = _SADD32 (scan.z_offset_xyslope, scan.z_slope_max);
        else if (d_tmp < -scan.z_slope_max) // limit dn
                scan.z_offset_xyslope = _SADD32 (scan.z_offset_xyslope, -scan.z_slope_max);
        else scan.z_offset_xyslope = s_xymult; // normally this should do it
                
#endif
        return 0;
}

// Generic Vector Probe Program between Section Processing Function calls moved out of real time regime:
extern void process_next_section ();

#pragma CODE_SECTION(idle_task_003, ".text:slow")
int idle_task_003(void){
        // Vector Probe Manager
        if (probe.pflg){
                if (!probe.ix) // idle task needs to work on section completion out of RT regime
                        process_next_section ();
                
                if (state.mode & MD_PID){
                        if (probe.vector)
                                if (probe.vector->options & VP_FEEDBACK_HOLD){
                                        STOP_RT_TASK (RT_TASK_FEEDBACK);
                                        z_servo.watch = 0; // must reset flag here, if task is not running, it is not managed
                                        return 1;
                                } else {
                                        START_RT_TASK_ODD (RT_TASK_FEEDBACK);
                                        return 1;
                                }
                }
        }
        return 0;
}

#pragma CODE_SECTION(idle_task_004, ".text:slow")
int idle_task_004(void){
        return 0;
}


/* PID-feedback on/off only via flag MD_PID -- nothing to do here */
        
#pragma CODE_SECTION(process_mode_set, ".text:slow")
void process_mode_set (int m){
        if (m & MD_PID){
                START_RT_TASK_ODD (RT_TASK_FEEDBACK);
        }
}

#pragma CODE_SECTION(process_mode_clr, ".text:slow")
void process_mode_clr (int m){
        if (m & MD_PID){
                STOP_RT_TASK (RT_TASK_FEEDBACK);
                z_servo.watch = 0; // must reset flag here, if task is not running, it is not managed
        }
}

#pragma CODE_SECTION(idle_task_005, ".text:slow")
int idle_task_005(void){
        /* state change request? */
        if (state.set_mode){
                state.mode |= state.set_mode;
                process_mode_set (state.set_mode);
                state.set_mode = 0;
                return 1;
        }
        if (state.clr_mode){
                state.mode &= ~state.clr_mode;
                process_mode_clr (state.clr_mode);
                state.clr_mode = 0;
                return 1;
        }
        /* (test) tool for log execution */
        if (feedback_mixer.exec){
                feedback_mixer.lnx = calc_mix_log (feedback_mixer.x, feedback_mixer.I_offset);
                feedback_mixer.exec = 0;
                return 1;
        }
        return 0;
}

#pragma CODE_SECTION(idle_task_006, ".text:slow")
int idle_task_006(void){
        /* DSP level setpoint calculation for log mode */
        int i;
        for (i=0; i<4; ++i){
                if (feedback_mixer.setpoint[i] != mix_setpoint_old[i]){ // any change?
                        feedback_mixer.setpoint_log[i] = calc_mix_log (feedback_mixer.setpoint[i], feedback_mixer.I_offset);
                        mix_setpoint_old[i] = feedback_mixer.setpoint[i];
                        return 1; // next call, next index!
                }
        }
        return 0;
}

#pragma CODE_SECTION(idle_task_007, ".text:slow")
int idle_task_007(void){
        /* module signal configuration management */
        if (sig_mon.mindex >= 0){
                if (sig_mon.signal_id < 0)
                        query_signal_input ();  // read back a input signal configuration
                else
                        adjust_signal_input ();  // adjust a input signal configuration
                return 1;
        }		
        return 0;
}

#pragma CODE_SECTION(idle_task_008, ".text:slow")
int idle_task_008(void){
        /* Start Offset Moveto ? */
        if (move.start && !autoapp.pflg){
                init_offset_move ();
#ifdef MIN_PAC_BUILD
                START_ID_TASK (ID_TASK_MOVE_AREA_SCAN);
#endif
                return 1;
        }
        return 0;
}

void switch_rt_task_areascan_to_probe (void){
        if (!probe.pflg){
#ifdef MIN_PAC_BUILD
                STOP_ID_TASK (ID_TASK_MOVE_AREA_SCAN);
#else
                STOP_RT_TASK (RT_TASK_AREA_SCAN);
#endif
                probe.start = 1; //init_probe ();
        }
}

#pragma CODE_SECTION(idle_task_009, ".text:slow")
int idle_task_009(void){
        /* Start/Stop/Pause/Resume Area Scan ? */

        if (scan.start == APPLY_NEW_ROTATION){
                scan.start = 0;
                copy32 (scan.rotm, scan.rotmatrix, 4); // copy scan.rotm to private copy used for operation
                return 1;
        }
        
        if (scan.start && !autoapp.pflg){
                init_area_scan (scan.start);
#ifdef MIN_PAC_BUILD
                START_ID_TASK (ID_TASK_MOVE_AREA_SCAN);
#else
                START_RT_TASK_EVEN (RT_TASK_AREA_SCAN);
#endif
                return 1;
        }
        
        switch (scan.stop){
        case 0: return 0;
        case AREA_SCAN_STOP:
                scan.stop = 0;
                finish_area_scan ();
#ifdef MIN_PAC_BUILD
                STOP_ID_TASK (ID_TASK_MOVE_AREA_SCAN);
#else
                STOP_RT_TASK (RT_TASK_AREA_SCAN);
#endif
                return 1; // Stop/Cancel/Abort scan
        case AREA_SCAN_PAUSE:
                scan.stop = 0;
                if (scan.pflg & AREA_SCAN_RUN){
#ifdef MIN_PAC_BUILD
                        STOP_ID_TASK (ID_TASK_MOVE_AREA_SCAN);
#else
                        STOP_RT_TASK (RT_TASK_AREA_SCAN);
#endif
                }
                return 1; // Pause Scan
        case AREA_SCAN_RESUME: 
                scan.stop = 0;
                if (scan.pflg & AREA_SCAN_RUN){
#ifdef MIN_PAC_BUILD
                        START_ID_TASK (ID_TASK_MOVE_AREA_SCAN);
#else
                        START_RT_TASK_EVEN (RT_TASK_AREA_SCAN);
#endif
                }
                return 1; // Resume Scan from Pause, else ignore
        default: return 0;
        }
}

#pragma CODE_SECTION(idle_task_010, ".text:slow")
int idle_task_010(void){
        int i;
        /* LockIn run free and Vector Probe control manager */
        if (probe.start == PROBE_RUN_LOCKIN_FREE){
                init_lockin ();
                START_RT_TASK_ALWAYS (RT_TASK_LOCKIN);
                return 1;
        }
        // ELSE:
        if (probe.start && !probe.pflg && !autoapp.pflg){
                if (scan.pflg)
#ifdef MIN_PAC_BUILD
                        STOP_ID_TASK (ID_TASK_MOVE_AREA_SCAN);
#else
                        STOP_RT_TASK (RT_TASK_AREA_SCAN);
#endif
                init_probe_fifo (); // reset probe fifo!
                init_probe ();
                START_RT_TASK_EVEN (RT_TASK_VECTOR_PROBE);
                return 1;
        }
        
        if (probe.stop == PROBE_RUN_LOCKIN_FREE){
                STOP_RT_TASK (RT_TASK_LOCKIN);
                return 1;
        }
        // ELSE:
        if (probe.stop){
                STOP_RT_TASK (RT_TASK_VECTOR_PROBE);
                stop_probe ();
                // make sure it's back on if enabled
                if (state.mode & MD_PID)
                        START_RT_TASK_ODD (RT_TASK_FEEDBACK);
                if (scan.pflg)
#ifdef MIN_PAC_BUILD
                        START_ID_TASK (ID_TASK_MOVE_AREA_SCAN);
#else
                        START_RT_TASK_EVEN (RT_TASK_AREA_SCAN);
#endif
                return 1;
        }
        // else if probe running, check on FB manipulations
        if (probe.pflg){
                if (probe.vector)
                        if (probe.vector->options & VP_GPIO_SET){
                                i =  (probe.vector->options & VP_GPIO_MSK) >> 16;
                                if (probe.gpio_setting != i){ // update GPIO!
                                        CR_generic_io.gpio_data_out = (DSP_UINT16)i;
                                        WRITE_GPIO_0 (CR_generic_io.gpio_data_out);
                                        //WR_GPIO (GPIO_Data_0, &CR_generic_io.gpio_data_out, 1);
                                        probe.gpio_setting = i;
                                }
                                return 1;
                        }
        }
        return 0;
}

#pragma CODE_SECTION(idle_task_011, ".text:slow")
int idle_task_011(void){
        /* Start Autoapproach/run Mover ? */
        if (autoapp.start && !probe.pflg && !scan.pflg){
                init_autoapp ();
                START_RT_TASK (RT_TASK_WAVEPLAY_PL);
                return 1;
        }
        if (autoapp.stop){
                STOP_RT_TASK (RT_TASK_WAVEPLAY_PL);
                stop_autoapp ();
                return 1;
        }
        return 0;
}

#pragma CODE_SECTION(idle_task_012, ".text:slow")
int idle_task_012(void){
        /* Start CoolRunner IO pulse ? */
        if (CR_out_pulse.start){
                init_CR_out_pulse ();
                START_RT_TASK (RT_TASK_WAVEPLAY_PL);
                return 1;
        }
        if (CR_out_pulse.stop){
                STOP_RT_TASK (RT_TASK_WAVEPLAY_PL);
                stop_CR_out_pulse ();
                return 1;
        }
        return 0;
}

#pragma CODE_SECTION(idle_task_013, ".text:slow")
int idle_task_013(void){
        /* Control GPIO pins ? */
        if (CR_generic_io.start){
                switch (CR_generic_io.start){
                case 1: WR_GPIO (GPIO_Data_0, &CR_generic_io.gpio_data_out, 1); break; // write port (0..15 back of the box)
                case 2: WR_GPIO (GPIO_Data_0, &CR_generic_io.gpio_data_in, 0); break; // read port (0..15 back of the box)
                case 3: WR_GPIO (GPIO_Dir_0, &CR_generic_io.gpio_direction_bits, 1); break; // reconfigure port back of the box	
#ifdef ENABLE_SCAN_GP53_CLOCK
                case 10: GPIO_CLR_DATA23 = (CR_generic_io.gpio_direction_bits&0x00E0)<<16; break; // CLR DSP GP pins 53/54/55
                case 11: GPIO_SET_DATA23 = (CR_generic_io.gpio_direction_bits&0x00E0)<<16; break; // SET DSP GP pins 53/54/55
                case 20: CLR_DSP_GP53; break;
                case 21: SET_DSP_GP53; break;
                case 22: CLR_DSP_GP54; break;
                case 23: SET_DSP_GP54; break;
                case 24: CLR_DSP_GP55; break;
                case 25: SET_DSP_GP55; break;
                case 30: configure_DSP_GP_PINS(); break; // configure for DSP GPIO pins 53,54,55 usage
			//case 99: *(volatile unsigned short*)(GPIO_Data_0) = CR_generic_io.gpio_data_out; break;
#endif
                default: break;
                }
                CR_generic_io.start=0;
                return 1;
        }
        return 0;
}

#pragma CODE_SECTION(idle_task_014, ".text:slow")
int idle_task_014(void){
#ifdef USE_PLL_API
        /* PAC/PLL controls */
        if (PLL_lookup.start){
                switch (PLL_lookup.start){
                case 1: ChangeFreqHL(); break;
                case 2: OutputSignalSetUp_HL(); break;
                case 3: TestPhasePIResp_HL(); break;
                case 4: TestAmpPIResp_HL(); break;
                case 99: StartPLL(5,7); break;
                }
                PLL_lookup.start = 0;
                return 1;
        }
#endif
        return 0;
}

#pragma CODE_SECTION(idle_task_015, ".text:slow")
int idle_task_015(void){
#ifdef RECONFIGURE_DSP_SPEED
        // DSP SPEED and McBSP control
        if (state.DSP_speed[0] != state.DSP_speed[1]){
                switch (state.DSP_speed[1]){
                case 1001: // McBSP ON: ENABLE DEFAULT
                        InitMcBSP0_in_RP_FPGA_Mode(8, 99); // DEFAULT SETUP -- Configure McBSP0 port for McBSP 8x32bit word frames, CTRL_FSG mode is default, 1MHz CLK
                        break;
                case 1002: // McBSP in RESET
                        ResetMcBSP0(); // put McBSP in RESET (STOP)
                        break;
                case 1003: // TEST, SCHEDULING SCHEME RIGHT AWAY or DELAYED AT END OF dataprocess (default)
                        DebugMcBSP0(0x10); // DEBUG OFF, schedule at end of dataprocess
                        break;
                case 1004: // DEBUG OFF (default)
                        DebugMcBSP0(0); // DEBUG OFF
                        break;
                case 1005:
                        DebugMcBSP0(1); // DEBUG ON, Level 1
                        break;
                case 1006:
                        DebugMcBSP0(2); // DEBUG ON, Level 2
                        break;
                case 1007:
                        DebugMcBSP0(3); // DEBUG ON, Level 3
                        break;
#define MCBSP_TESTING
#ifdef MCBSP_TESTING
                case 1011: // configure_McBSP_N 1010+N
                        InitMcBSP0_in_RP_FPGA_Mode(1,0); // Configure McBSP0 port for McBSP 1x32bit word frames, keep clock
                        break;
                case 1012:
                        InitMcBSP0_in_RP_FPGA_Mode(2,0); // Configure McBSP0 port for McBSP 2x32bit word frames, keep clock
                        break;
                case 1013:
                        InitMcBSP0_in_RP_FPGA_Mode(4,0); // Configure McBSP0 port for McBSP 4x32bit word frames, keep clock
                        break;
                case 1014:
                        InitMcBSP0_in_RP_FPGA_Mode(8,0); // Configure McBSP0 port for McBSP 8x32bit word frames, keep clock
                        break;
                case 1015:
                        InitMcBSP0_in_RP_FPGA_Mode(16,0); // Configure McBSP0 port for McBSP 16x32bit word frames, keep clock
                        break;
#endif
                case 1020: // configure_McBSP_M 1020+M
                        InitMcBSP0_in_RP_FPGA_Mode(8, 199); // 0.5MHz
                        break;
                case 1021:
                        InitMcBSP0_in_RP_FPGA_Mode(8, 99); // 1MHz
                        break;
                case 1022:
                        InitMcBSP0_in_RP_FPGA_Mode(8, 49); // 2MHz
                        break;
                case 1023:
                        InitMcBSP0_in_RP_FPGA_Mode(8, 24); // 4MHz
                        break;
                case 1024:
                        InitMcBSP0_in_RP_FPGA_Mode(8, 19); // 5MHz
                        break;
                case 1025:
                        InitMcBSP0_in_RP_FPGA_Mode(8, 15); // 6.25MHz
                        break;
                case 1026:
                        InitMcBSP0_in_RP_FPGA_Mode(8, 11); // 8.33MHz
                        break;
                case 1027:
                        InitMcBSP0_in_RP_FPGA_Mode(8, 9); // 10MHz
                        break;
                case 1028:
                        InitMcBSP0_in_RP_FPGA_Mode(8, 7); // 12.5MHz
                        break;
                case 1029:
                        InitMcBSP0_in_RP_FPGA_Mode(8, 6); // 14.28MHz
                        break;
                case 1030:
                        InitMcBSP0_in_RP_FPGA_Mode(8, 5); // 16.67MHz
                        break;
                case 1031:
                        InitMcBSP0_in_RP_FPGA_Mode(8, 4); // 20MHz
                        break;
                case 1032:
                        InitMcBSP0_in_RP_FPGA_Mode(8, 3); // 25MHz
                        break;
                case 1033:
                        InitMcBSP0_in_RP_FPGA_Mode(8, 2); // 33MHz *** duty cycle far off 50:50!!! DO NOT USE
                        break;
                case 1034:
                        InitMcBSP0_in_RP_FPGA_Mode(8, 1); // 50MHz
                        break;
                case 1035:
                        InitMcBSP0_in_RP_FPGA_Mode(8, 0); // 100MHz
                        break;

                case 688:
                        stop_Analog810 (); // Stop AICs now again -- testing only
                        dsp_688MHz();
                        start_Analog810 ();
                        state.DSP_speed[1]=state.DSP_speed[0]=688;
                        break;
                default:
                        stop_Analog810 (); // Stop AICs now again -- testing only
                        dsp_590MHz();
                        start_Analog810 ();
                        state.DSP_speed[1]=state.DSP_speed[0]=590;
                        break;
                }
                state.DSP_speed[1]=state.DSP_speed[0]; // or no actual speed change!
                return 1;
        }
#endif
        return 0;
}

#pragma CODE_SECTION(idle_task_016, ".text:slow")
int idle_task_016(void){
#ifdef AIC_STOPMODE_ENABLE_XX

        /* DANGER!!! Analog reconfiguration may cause freeze issues if used wrong --> ISR */
        if (state.mode & MD_AIC_STOP){
                if (!AIC_stop_mode){
                        AIC_stop_mode = 1;
                        sleepcount = 0;

                        /* Stop Analog810 -- Out/In may be undefined while stopped */
                        stop_Analog810 ();
                }
                if (++sleepcount > 50){
                        sleepcount = 0;
                        dataprocess();
                }
        } else {
                if (AIC_stop_mode){
                        AIC_stop_mode = 0;

                        /* ReInit and Start Analog810 */
                        FreqDiv   = a810_config.freq_div;     // default: =10 75kHz *** 5 ... 65536  150kHz at N=5   Fs = Clk_in/(N * 200), Clk_in=150MHz
                        ADCRange  = a810_config.adc_range;    // default: =0 0: +/-10V, 1:+/-5V
                        QEP_ON    = a810_config.qep_on;       // default: =1 (on) manage QEP counters
                        start_Analog810 ();
                }
        }
#endif
        return 0;
}

#pragma CODE_SECTION(idle_task_017, ".text:slow")
int idle_task_017(void){
        int i;
// ============================================================
// PROCESS MODULE: SIGNAL MONITOR data copy processing
// ============================================================
        for (i=0; i<NUM_MONITOR_SIGNALS; ++i)
                sig_mon.signal[i] = *sig_mon.signal_p[i];

        return 0;
}

#pragma CODE_SECTION(idle_task_018, ".text:slow")
int idle_task_018(void){
// ============================================================
// PROCESS MODULE: McBCP update async
// ============================================================
        if (!scan.pflg && !probe.pflg && !state.dp_task_control[9].process_flag){
                initiate_McBSP_transfer (0);
                return 1;
        }
        return 0;
}

inline void Mrun_servo_timestep (SERVO *servo){
	long long tmp;
	servo->i_sum = _SAT32 ((long)servo->i_sum + (long)( ((long long)servo->delta * (long long)servo->ci) >> (15+23)) );
	tmp = (long)((long)servo->i_sum + (((long long)servo->delta * (long long) servo->cp) >> (15+23)));
	// make both output polarities available
	servo->control = _SAT32 (tmp);
	servo->neg_control = _SAT32 (-tmp);
	servo->watch = servo->control >> 16;
}

#pragma CODE_SECTION(idle_task_019, ".text:slow")
int idle_task_019(void){
// ============================================================
// PROCESS MODULE: MOTOR SERVO on McBSP -- optimized for max regualtion error (saturating) of 14.9Hz
// experimental for SQDM bias tracking via M-Servo.
// Use Python DSP Manager and Task Control to enable this timer task (#19)
// and disable the above (#18) as conflicting potentially and obsoleted
// Also need to configure adding of M-SERVO-WATCH signal (16bit mapped) servo control value to Bias
// with for example a scaling (SMAC-A/B type output mapping)
// using for example the "Noise-Amplidute Signal unless not used otherwise
// ============================================================
        initiate_McBSP_transfer (0);
	m_servo.delta = _SSHL32(_SAT32((long)m_servo.setpoint - (long)analog.McBSP_FPGA[1]), 10);
        // "Q23" input and setpoint, SAT difference
        //-- if m_servo.input is McBSP Freq. scaling is 125e6Hz / (1<<44 -1)
        //-- i.e. 1Hz = 140737.488  (1<<21 = 2097152  ~15Hz to fill 31bit needs << 10)
        // (m_servo.setpoint - analog.McBSP_FPGA[1]) << 10  gives max delta (saturated) at 10Hz
        
	Mrun_servo_timestep (&m_servo); // delta * CI  >> (15+23)   --->   15.16
        return 0;
}

#pragma CODE_SECTION(idle_task_020, ".text:slow")
int idle_task_020(void){
        initiate_McBSP_transfer (0);
	m_servo.watch = analog.McBSP_FPGA[7] >> 16; // map FPGA McBSP[7] (Bias Control)
        return 0;
}

#pragma CODE_SECTION(idle_task_021, ".text:slow")
int idle_task_021(void){
// ============================================================
// PROCESS MODULE: smart RT task supervisor
// ============================================================

#if 0

        // Auto LockIn management -- need to check for actual channel assignments...
        if (srcs & 0x01000) // DataSrcC1 --> LockIn1stA [default maped signal]
                rt_fifo_push_data[13][rt_fifo_i] = *scan.src_input[0];
        if (srcs & 0x02000) // DataSrcD1 --> LockIn2ndA [default maped signal]
                rt_fifo_push_data[14][rt_fifo_i] = *scan.src_input[1];
        if (srcs & 0x04000) // DataSrcE1 --> LockIn0 [default maped signal]
                rt_fifo_push_data[15][rt_fifo_i] = *scan.src_input[2];
        if (srcs & 0x08000) // "DataSrcF1" last CR Counter count [default maped signal]
                rt_fifo_push_data[16][rt_fifo_i] = *scan.src_input[3];

        probe.LockIn_input[0] = &feedback_mixer.FB_IN_processed[0]; // defaults for LockIn Inputs
	probe.LockIn_input[1] = &feedback_mixer.FB_IN_processed[0];
	probe.src_input[0] = &probe.LockIn_1stB; // defaults for VP input mapping
	probe.src_input[1] = &probe.LockIn_2ndA;
	probe.src_input[2] = &probe.LockIn_2ndA;
	probe.src_input[3] = &analog.counter[0];
	probe.prb_output[0] = &scan.xyz_vec[i_X]; // defaults for VP output mapping
	probe.prb_output[1] = &scan.xyz_vec[i_Y];
	probe.prb_output[2] = &move.xyz_vec[i_X];
	probe.prb_output[3] = &move.xyz_vec[i_Y];
#endif

        return 0;
}

#pragma CODE_SECTION(idle_task_022, ".text:slow")
int idle_task_022(void){
        return 0;
}

#pragma CODE_SECTION(idle_task_023, ".text:slow")
int idle_task_023(void){
        return 0;
}

#pragma CODE_SECTION(idle_task_024, ".text:slow")
int idle_task_024(void){
        return 0;
}

#pragma CODE_SECTION(idle_task_025, ".text:slow")
int idle_task_025(void){
        return 0;
}

#pragma CODE_SECTION(idle_task_026, ".text:slow")
int idle_task_026(void){
        return 0;
}

#pragma CODE_SECTION(idle_task_027, ".text:slow")
int idle_task_027(void){
        return 0;
}

#pragma CODE_SECTION(idle_task_028, ".text:slow")
int idle_task_028(void){
        if (!probe.pflg){
                /* (re) Adjust Bias, Zpos ? */
                probe.Upos = analog.bias_adjust; // probe.Upos is default master for BIAS at all times, following user control or VP comand
                if (analog.bias_adjust != analog.bias[scan.section]){
                        //run_bias_adjust ();
			analog.bias_adjust = analog.bias[scan.section];
                        return 1;
                }
                if ((state.mode & MD_ZPOS_ADJUSTER) && probe.Zpos != 0){ // enable Zpos adjusting normally!
                        run_Zpos_adjust ();
                        return 1;
                }
        }
        return 0;
}

#pragma CODE_SECTION(idle_task_029, ".text:slow")
int idle_task_029(void){
        return 0;
}


// RMS buffer
#define RMS_N2           8
#define RMS_N            (1 << RMS_N2)
int     rms_pipi = 0;       /* Pipe index */
int     rms_I_pipe[RMS_N] = 
{ 
        //   16       32                 64                                   128                                                                        256
	0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0
};
DSP_INT64    rms_I2_pipe64[RMS_N] = 
{ 
        //   16       32                 64                                   128                                                                        256
	0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0
};

#pragma CODE_SECTION(idle_task_030, ".text:slow")
int idle_task_030(void){
        DSP_INT32  d_tmp;
        long      tmpsum  = 0;
        DSP_INT64 rmssum2 = 0L;
        DSP_INT64 rmssum  = 0L;

// ============================================================
// PROCESS MODULE: RMS -- average and rms computations via FIR
// ============================================================
	// Average and RMS computations
	d_tmp = *analog.avg_input >> 16; // 32bit (QS15.16 input) -- scale to fit sum 
	analog.avg_signal = _SADD32 (analog.avg_signal, d_tmp);
	analog.avg_signal = _SSUB32 (analog.avg_signal, rms_I_pipe[rms_pipi]);
	rms_I_pipe[rms_pipi] = d_tmp;
						
	tmpsum = _SSUB32 (d_tmp, _SSHL32 (analog.avg_signal, -RMS_N2));
	rmssum2 = tmpsum;
	rmssum2 *= rmssum2;
	rmssum  += rmssum2; 
	rmssum  -= rms_I2_pipe64[rms_pipi];
	analog.rms_signal = rmssum >> 32;
	rms_I2_pipe64[rms_pipi] = rmssum;

	// now tricky -- taking chances of mult OVR if RMS is too big, designed for read small noise with offset eliminated.
	// tmpsum = _SMPY32 (tmpsum, tmpsum);
	// analog.rms_signal = _SADD32 (analog.rms_signal, tmpsum);
	// analog.rms_signal = _SSUB32 (analog.rms_signal, rms_I2_pipe[rms_pipi]);
	// rms_I2_pipe[rms_pipi]  = tmpsum;
				
	if (++rms_pipi == RMS_N) 
		rms_pipi = 0;
	// RMS, AVG results normalizsation on host level!

        return 1;
}

/* Auxillary Random Number Generator */

#define RNG_a 16807         /* multiplier */
#define RNG_m 2147483647L   /* 2**31 - 1 */
#define RNG_q 127773L       /* m div a */
#define RNG_r 2836          /* m mod a */

DSP_INT32 randomnum = 1L;

#pragma CODE_SECTION(idle_task_031, ".text:slow")
int idle_task_031(void){
        DSP_INT32 lo, hi;
        // ============================================================
        // PROCESS MODULE: NOISE GENERATOR (RANDOM NUMBER)
        // ============================================================

        lo = _SMPY32 (RNG_a, randomnum & 0xFFFF);
        hi = _SMPY32 (RNG_a, (DSP_UINT32)randomnum >> 16);
        lo += (hi & 0x7FFF) << 16;
        if (lo > RNG_m){
                lo &= RNG_m;
                ++lo;
        }
        lo += hi >> 15;
        if (lo > RNG_m){
                lo &= RNG_m;
                ++lo;
        }
        randomnum = lo;
	analog.noise = randomnum;

        return 1;
}

#pragma CODE_SECTION(idle_task_032, ".text:slow")
int idle_task_032(void){
        // 1s Timer
        state.BLK_count_seconds++;
        if (state.DSP_seconds > 0)
                state.DSP_seconds--;
        else{
                state.DSP_seconds = 59;
                state.BLK_count_minutes++;
        }
        return 1;
}


typedef int (*func_ptr)(void);

func_ptr idle_task_list[NUM_IDLE_TASKS] = {
        idle_task_001, idle_task_002, idle_task_003, idle_task_004,
        idle_task_005, idle_task_006, idle_task_007, idle_task_008,
        idle_task_009, idle_task_010, idle_task_011, idle_task_012,
        idle_task_013, idle_task_014, idle_task_015, idle_task_016,
        idle_task_017, idle_task_018, idle_task_019, idle_task_020,
        idle_task_021, idle_task_022, idle_task_023, idle_task_024,
        idle_task_025, idle_task_026, idle_task_027, idle_task_028,
        idle_task_029, idle_task_030, idle_task_031, idle_task_032
};



int run_next_priority_job(void){
        static int iip=0;
        int i;
        for (i=iip; i<NUM_IDLE_TASKS; i++){
                if (state.id_task_control[i].process_flag & 0x10){
                        state.id_task_control[i].process_flag |= 1;
                        if (idle_task_list [i] ()){
                                state.id_task_control[i].timer_next = state.DSP_time; // since last data process end
                                state.id_task_control[i].exec_count++; // was actually processing, count n calls in flag of idle tasks
                                iip = (i+1) & (NUM_IDLE_TASKS-1);
                                return 1;
                        }
                        state.id_task_control[i].process_flag &= 0xf0;
                }
        }
        iip=0;
        return 0;
}

// NOTE: CHECK FOR state.DSP_time looping conditon "rep timer" mode
int run_next_sync_job(void){
        static int iip=0;
        int i;
        long delta;
        for (i=iip; i<NUM_IDLE_TASKS; i++){
                if (state.id_task_control[i].process_flag & 0x20){
                        delta = (long)state.id_task_control[i].timer_next - (long)state.DSP_time;
                        if (delta < 0 || delta > 0x7FFFFFFFL){
                                state.id_task_control[i].process_flag |= 1;
                                state.id_task_control[i].timer_next = state.DSP_time + state.id_task_control[i].interval; // next time due, too close to now
                                if (idle_task_list [i] ()){
                                        state.id_task_control[i].exec_count++; // was actually processing, count n calls in flag of idle tasks
                                        iip = (i+1) & (NUM_IDLE_TASKS-1);
                                        return 1;
                                }
                                state.id_task_control[i].process_flag &= 0xf0;
                        }
                }
        }
        iip=0;
        return 0;
}

// NOTE: CHECK FOR state.DSP_time looping conditon "clock" mode
#pragma CODE_SECTION(run_next_sync_job_precision, ".text:slow")
int run_next_sync_job_precision(void){
        static int iip=0;
        int i;
        long delta;
        for (i=iip; i<NUM_IDLE_TASKS; i++){
                if (state.id_task_control[i].process_flag & 0x40){
                        delta = (long)state.id_task_control[i].timer_next - (long)state.DSP_time;
                        if (delta < 0 || delta > 0x7FFFFFFFL){
                                state.id_task_control[i].process_flag |= 1;
                                if ( -delta < state.id_task_control[i].interval && !(delta > 0))
                                        state.id_task_control[i].timer_next += state.id_task_control[i].interval; // next time due -- exact
                                else
                                        state.id_task_control[i].timer_next = state.DSP_time + state.id_task_control[i].interval; // next time due, too close to now
                                if (idle_task_list [i] ()){
                                        state.id_task_control[i].exec_count++; // was actually processing, count n calls in flag of idle tasks
                                        iip = (i+1) & (NUM_IDLE_TASKS-1);
                                        return 1;
                                }
                                state.id_task_control[i].process_flag &= 0xf0;
                        }
                }
        }
        iip=0;
        return 0;
}


void dsp_idle_loop (void){
	for(;;){	/* forever !!! */

                /* LOW PRIORITY: Process Priority Idle Tasks */
                if (run_next_priority_job())
                        continue;
                
                /* LOW PRIORITY: Process Timed Idle Tasks */
                if (run_next_sync_job())
                        continue;
                
                /* LOW PRIORITY: Precision Process Timed Idle Tasks -- intervall >> 100 please */
                if (run_next_sync_job_precision())
                        continue;
                
	} /* repeat idle loop forever... */
}
