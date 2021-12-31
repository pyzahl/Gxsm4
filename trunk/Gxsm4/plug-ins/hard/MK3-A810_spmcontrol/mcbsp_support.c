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

#include "SR3_Reg.h"
#include "g_intrinsics.h"
#include "FB_spm_dataexchange.h"
#include "mcbsp_support.h"

#define Time1_us 84 // In the assembler function _wait, the value to set to obtain 1 us is 84. It is 7 CPU cycles * 84 at 589 MHz.
                    // *** =99 for 688 MHz

#define McBSP_Tx_Bit 0x0080 // INT7
#define McBSP_Rx_Bit 0x0100 // INT8
#define McBSP_RxTx_Bit (McBSP_Rx_Bit | McBSP_Tx_Bit) // INT8 and INT7


extern ANALOG_VALUES    analog;

// advanced debugging
// #define DEBUG_TXRX_COUNTING

int McBSP_words_per_frame = 8;
int McBSP_frame_element = 0;
int McBSP_word_count = 0;
int McBSP_enabled = 0;
int McBSP_clkdiv = 99; // Serial Clock 1MHz
int McBSP_data[16] = { 0, 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 }; // McBSP TX data array

unsigned int McBSP_transmitted_words = 0;
unsigned int McBSP_received_words = 0;

unsigned int McBSP_debug_mode=0;

unsigned int McBSP_schedule_pending_count = 0;

#pragma CODE_SECTION(SetMcBSPwords, ".text:slow")
void SetMcBSPwords(int n){
        if (n >= 1 && n <= 16)
                McBSP_words_per_frame = n; 
}

#pragma CODE_SECTION(SetMcBSPclock, ".text:slow")
// McBSP (McBSP) CLKGDV=div,  CLK = 100MHz / (CLKGDV+1)
void SetMcBSPclock(int div){
        if (div >= 0 && div <= 255)
                McBSP_clkdiv = div; 
}

#pragma CODE_SECTION(ResetMcBSP0, ".text:slow")
void ResetMcBSP0()
{
	MCBSP0_SPCR=(3<<CLKSTPBit);	// Put the McBSP0 in reset
        McBSP_enabled = 0;
        McBSP_words_per_frame = 8;
        McBSP_word_count = 0;
        McBSP_debug_mode = 0;
        McBSP_transmitted_words = 0;
        McBSP_received_words = 0;
        McBSP_frame_element = McBSP_words_per_frame;
        McBSP_data[0] = 0;
        McBSP_schedule_pending_count = 0;
}

#pragma CODE_SECTION(InitMcBSP0_in_RP_FPGA_Mode, ".text:slow")
// wpf: words per frame: 1..8 (16 experimental)
// div: 0: no change, 1..255, see SetMcBSPclock(div)
void InitMcBSP0_in_RP_FPGA_Mode(int wpf, int div)
{
        SetMcBSPwords (wpf);
        SetMcBSPclock (div);
        
        // DisableInts_SDB;
        CSR = ~0x1 & CSR; // Disable INTs.
        
	// STEP 1: Power up the SP

	CFG_VDD3P3V_PWDN &= ~SetSPPD; // 0x80 : bit SP(mcbsp0,1)

	// STEP 2

        // ====== McBSP Serial Port Control Register (SPCR)
	//MCBSP0_SPCR=(0<<CLKSTPBit);	// Put the McBSP0 in reset, regular McBSP mode, clock is running

        // CLKSTP=3 Clock starts with rising edge with delay [SPI mode]
        // MCBSP_SPCR_DLB // DLBBit [digital loopback test]
	// MCBSP0_SPCR=(3<<CLKSTPBit) + (0 << DLBBit);	// Put the McBSP0 in reset // 3= SPI (Clock Stop Mode)
	MCBSP0_SPCR = (0 << CLKSTPBit) + (0 << DLBBit);	// Put the McBSP0 in reset, McBSP default mode

	// STEP 3

        // Configure:
        // ====== Pin Control Register PCR
        // CLKRPBit   0 = 0   1: rising edge!    0: receive data sampled on falling edge of CLKR (input)
        // CLKXPBit   1 = 0   rising clock pol (transmit on rising edge of CLKX)
        // FSRPBit    2 = 0   receive fram pol pos
        // FSXPBit    3 = 0   pos Frame Pol
        // SCLKME     7 = 0  (sample rate gen...)
        // CLKRMBit   8 = 0  CLKR is input (returned clock!! need this)
        // CLKXMBit   9 = 1  McBSP master and generates clkx for slave and for its receive clock 
        // FSRMBit   10 = 0  0: is derived from external source. 1: internally, FSR is output
        // FSXMBit   11 = 1  Frame Sync Generation by FSGM bit in SRGM, 0: is derived from external source.
        // RIOENBit  12 = 0
        // XIOENBit  13 = 0   DX, FSX, CLKX active

        // CLKX and FSX must be output
        // -> CLKXM=1, FSXM=1
        // CLKR and FSR must be input (reference clock/frame from clock/frame return over wire):
        // -> CLKRM=0, FSRM=0 
	MCBSP0_PCR = (0 << CLKRPBit) + (1 << PCR_CLKXMBit) + (1 << PCR_FSXMBit) + (0 << PCR_CLKXPBit) + (0 << PCR_CLKRPBit) + (0 << PCR_FSXPBit);
        
        // ===== Sample Rate Generator Register SRGR
	// Set the McBSP (clock: Internal clock at SYSClk3 (CPU (589E6)/6): diviser /7: 14.024 MHz)
        // FSGM=1 0: Transmit FSX is generated on every DXR->XSR copy, 1: FSX is driven by sample rate generator frame sync signal (FSG)
        // CLKSP=GSYNC=0 (don't care in internal input clock mode)
        // CLKSM=1 (Internal input clock)
        // FPER= (8x32-bit words by frame)  8*32-1   -- ??? total frame length for all words ???
        // FWID=0 =1: 2 CLK WIDE FS Pulse
        // CLKGDV=6 (divider at 7 to generate 14.024 MHz) CLK = 100MHz / (CLKGDV+1)
        // (99<<CLKGDVBit) // 1 MHz clock
        // (49<<CLKGDVBit) // 2 MHz clock
        // (12<<CLKGDVBit) // 8 MHz clock **
        // (24<<CLKGDVBit) // 4 MHz clock
        // (6<<CLKGDVBit)  // 16 MHz clock,   (6<<CLKGDVBit); //  20 MHz ???
	//MCBSP0_SRGR=(1<<CLKSMBit) + ((8*32-1)<<FPERBit)+ (99<<CLKGDVBit) + (0 << FSGMBit) + (1 << FWIDBit); // 1MHz
        if (wpf > 8)
                MCBSP0_SRGR = (1 << FSGMBit) + (1<<CLKSMBit) + ((16*32-1+8)<<FPERBit) + (McBSP_clkdiv<<CLKGDVBit) + (0 << FSGMBit) + (1 << FWIDBit); // FSGMBit=0: Transmit FSX is generated on every DXR->XSR copy
        else
                MCBSP0_SRGR = (1 << FSGMBit) + (1<<CLKSMBit) + (( 8*32-1+8)<<FPERBit) + (McBSP_clkdiv<<CLKGDVBit) + (0 << FSGMBit) + (1 << FWIDBit); // FSGMBit=0: Transmit FSX is generated on every DXR->XSR copy
        
        // ===== McBSP Receive Control Register (RCR) and Transmit Control Register (XCR)
	// want 32bit, 4x 32bit words default, up to 8 words
	MCBSP0_RCR=(1<<RDATDLYBit) + (5<<RWDLEN1Bit) + ((McBSP_words_per_frame-1) << RFRLEN1Bit);	// 32-bits word (phase1), 1 words by frame, Receive 1-bit delay
        MCBSP0_XCR=(1<<XDATDLYBit) + (5<<XWDLEN1Bit) + ((McBSP_words_per_frame-1) << XFRLEN1Bit);	// 32-bits word (phase1), 1 words by frame, Transmit 1-bit delay
        // RPHASE=XPHASE=0
        // RFRLEN1=XFRLEN1=0
        // RCOMPAND=0
        // RFIG=0
        // RDATDLY=XDATDLY=1
        // RFRLEN1=XFRLEN1=0 1 word of RWDLEN1-bits
        // XFRLEN1=XFRLEN1=0 1 word of XWDLEN1-bits
        // RWDLEN1=XWDLEN1=0 (8-bits) =5 (32-bits)
        // XWDLEN1=XWDLEN1=0 (8-bits) =5 (32-bits)

	// STEP 4: wait 2 clks (we do 10)

	// Wait for the synchronization with CLKS
	// (10 cyles of internal clock (12.288MHz): 1 us)

	wait(Time1_us);			// wait 1 us

	// STEP 5: GRST=1 Sample-rate generator enable (wait the double of step 4)

	MCBSP0_SPCR |= MCBSP_SPCR_GRST;
	wait(2*Time1_us);

	// STEP 6: XRST=1, wait like step 5 and XRST=0

	MCBSP0_SPCR |= MCBSP_SPCR_XRST;
	wait(2*Time1_us);
	MCBSP0_SPCR &= ~MCBSP_SPCR_XRST;

        // should be OK as set above, but make sure
        MCBSP0_SPCR &= ~(3 << XINTMBit); // make sure it is =0 for XINT driven by XRDY (end of word) and end of frame
        MCBSP0_SPCR &= ~(3 << RINTMBit); // make sure it is =0 for RINT driven by RRDY (end of word) and end of frame
        
	// STEP 7: Init EDMA (no DMA)

	// STEP 8: XRST=RRST=1 enable Transmitter and enable Receiver.

	MCBSP0_SPCR |= (MCBSP_SPCR_XRST+MCBSP_SPCR_RRST); 

	// 9: Nothing to do

	// 10: wait

	wait(2*Time1_us);

        // Int MUX configuration for RX and TX
	// tms320c6424.pdf p. 167
	// 48 MBXINT0 McBSP0 Transmit    = 0x30
	// 49 MBRINT0 McBSP0 Receive     = 0x31
	// 50 MBXINT1 McBSP1 Transmit    = 0x32
	// 51 MBRINT1 McBSP1 Receive     = 0x33

	// Init Mux1 and Mux2 for McBSP0 TX event MBXINT0 # d48 on (int7)  and RX MBRINT0 # d 49 on (int8)
	INTC_INTMUX1=(INTC_INTMUX1 & 0x00FFFFFF) | 0x30000000;		// Add the MBXINT1 event (51:30h) on INT7 ->TX
	INTC_INTMUX2=(INTC_INTMUX2 & 0xFFFFFF00) | 0x00000031;		// Add the MBRINT1 event (50:31h) on INT8 ->RX

        // DisableInts_SDB;
        // CSR = ~0x1 & CSR; // Disable INTs 

	IER &= ~McBSP_RxTx_Bit; // Disable RX/TX int

        // EnableInts_SDB;
        CSR = 1 | CSR; // Enable ints 

	ICR=McBSP_RxTx_Bit; // Clear flag

	// 11: FRST=1 (enable frame-syn generator)

        // Hack to FSG control:
        // ONLY SEND one FS on request for data!
        // not yet! Starts at transfer request. And is stopped after last word is send again.

        // MCBSP0_SPCR |= MCBSP_SPCR_FRST; // start FSG -- not yet!
        
        McBSP_word_count = 0;
        McBSP_enabled = 1;
        McBSP_frame_element = McBSP_words_per_frame;
        McBSP_schedule_pending_count = 0;

        analog.McBSP_FPGA[8] = MCBSP0_SPCR;
        analog.McBSP_FPGA[9] = MCBSP0_PCR;
       
	IER |= McBSP_RxTx_Bit; // Enable RX/TX int
}

void initiate_McBSP_transfer(unsigned int index){
        // set 1st data word
        McBSP_data[0] = index;

        // Is the McBSP coinfigured and enabled
        if (!McBSP_enabled)
                return;
        
        // transfer in progress -- too fast/overlapping, skip. => holding values.
        if (McBSP_frame_element < McBSP_words_per_frame){
                analog.McBSP_FPGA[8] = 0xff00ff00; // add overrun error mark
                McBSP_schedule_pending_count++;
                if (McBSP_schedule_pending_count > 100) // auto recover after 100 skipps
                        InitMcBSP0_in_RP_FPGA_Mode(0,0); // re initialize with last settings
                return;
        }
                
        McBSP_frame_element = 0; // reset receiver frame element count
        McBSP_word_count    = McBSP_words_per_frame-1; // remeining words to transfer after this one below
        MCBSP0_DXR_32BIT    = McBSP_data[0]; // copy Initial Word now (contains line/col or probe index only)

        // take FSG out of reset, start frame transmission, stopped again after last word is out!
        MCBSP0_SPCR |= MCBSP_SPCR_FRST; // start FSG!

        // Debug
        switch (McBSP_debug_mode & 0x0f){
        case 0: break;
        case 1:
                analog.McBSP_FPGA[8] = MCBSP0_SPCR & (  (1 << XSYNCERRBit) | (1 << XEMPTYBit) | (1 << XRDYBit)
                                                     | (1 << RSYNCERRBit) | (1 << RFULLBit)  | (1 << RRDYBit));
#ifdef DEBUG_TXRX_COUNTING
                analog.McBSP_FPGA[10] = McBSP_transmitted_words;
                analog.McBSP_FPGA[11] = McBSP_received_words;
#endif
                break;
        case 2:
#ifdef DEBUG_TXRX_COUNTING
                analog.McBSP_FPGA[6] = McBSP_transmitted_words;
                analog.McBSP_FPGA[7] = McBSP_received_words;
#endif
                analog.McBSP_FPGA[8] = McBSP_schedule_pending_count;
                break;
        case 3:
#ifdef DEBUG_TXRX_COUNTING
                analog.McBSP_FPGA[6] = McBSP_transmitted_words;
                analog.McBSP_FPGA[7] = McBSP_received_words;
#endif
                analog.McBSP_FPGA[8] = MCBSP0_SPCR;
                break;
        }
}

// McBSP [SPI] TX (Transmit)
interrupt void McBSP1TX_INT()
{
        if (McBSP_word_count){
                MCBSP0_DXR_32BIT = McBSP_data[McBSP_word_count--];
#ifdef DEBUG_TXRX_COUNTING
                McBSP_transmitted_words++;
#endif
                if (McBSP_word_count == 0)
                        MCBSP0_SPCR &= ~MCBSP_SPCR_FRST; // stop FSG
        }
}

// McBSP [SPI] RX (Receive)
interrupt void McBSP1RX_INT()
{
        if (McBSP_frame_element < McBSP_words_per_frame ){
                analog.McBSP_FPGA[McBSP_frame_element++] = MCBSP0_DRR_32BIT;
#ifdef DEBUG_TXRX_COUNTING
                McBSP_received_words++;
#endif
        }
}

#pragma CODE_SECTION(DebugMcBSP0, ".text:slow")
void DebugMcBSP0(int level)
{
        if (!McBSP_enabled)
                return;

        McBSP_transmitted_words = 0;
        McBSP_received_words = 0;
        McBSP_debug_mode=level;
}
