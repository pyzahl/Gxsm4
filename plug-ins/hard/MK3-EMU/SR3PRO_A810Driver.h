#ifndef __SR3PRO_A810DRIVER_H
#define __SR3PRO_A810DRIVER_H

/* ------------------------------------------------------------------------ *
 *  Constants definitions
 * ------------------------------------------------------------------------ */
                                                                             
#define AIC_Ctrl_Reg 	 	*( volatile unsigned short* )(0x46000000)	// Analog810 register in FPGA
#define RegFreqDiv_Fpga		*( volatile unsigned short* )(0x46000004)	// Frequency div address in FPGA (word address)

#define WrtReadAdr_Fpga		(0x46000002)	// Read/Write (word address)	in FPGA
#define QEP_Count_0_FIFO	(0x4600000A)	// QEP Fifo (bytes address) in FPGA
#define QEP_Count_1_FIFO	(0x4600000C)	// QEP Fifo (bytes address) in FPGA

#define AIC_Run	0x0001		// If bit 0 = 0 --> the AIC are in reset
							// If bit 0 = 1 --> the AIC are in operation
#define AIC_Range	0x0002	// If bit 1 = 1 --> the ADC Range is +-5V
	

/* ------------------------------------------------------------------------ *
 *  Structure definition                                                 *
 * ------------------------------------------------------------------------ */

struct iobuf_rec {
        DSP_INT16 min[8];
        DSP_INT16 mout[8];
};

#ifndef DSPEMU

/* ------------------------------------------------------------------------ *
 *  Variables declaration                                                   *
 * ------------------------------------------------------------------------ */

extern	struct iobuf_rec iobuf;
		
//  QEP counter 1 and 2 
       	
extern	short QEP_cnt[2];

// _FreqDiv allows the user to select the sampling frequency

extern volatile	short FreqDiv; // minimum 5
extern volatile	short ADCRange;	// 1: +-5V 0: +-10V
extern volatile	short QEP_ON; // 1: ON	0: OFF 

/* ------------------------------------------------------------------------ *
 *  Functions declaration                                                   *
 * ------------------------------------------------------------------------ */

extern void start_Analog810();
extern void stop_Analog810();
extern void dataprocess();

#endif
#endif
