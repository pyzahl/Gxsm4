/*___________________________________________________________________________

	Function headers go here
	
___________________________________________________________________________*/     

#include "SR3_Reg.h"

#ifndef DSPEMU
void main();
#else
int main (int argc, char *argv[]);
#endif


void setup_default_signal_configuration();
void adjust_signal_input ();
void query_signal_input ();
int store_configuration_to_flash();
int restore_configuration_from_flash();
void fill_magic();
void start_Analog810_HL();
void stop_Analog810_HL();
void enablePll1( Uint16 pll_mult );
void dsp_688MHz();
void dsp_590MHz();

#ifndef DSPEMU
#define NULL ((void *) 0)
#endif
