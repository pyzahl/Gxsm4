s/ dsp_signal_lookup = {/ = [/
s/#define DSP_SIGNAL_VECTOR_ELEMENT_INDEX_REQUEST_MSK/DSP_SIGNAL_VECTOR_ELEMENT_INDEX_REQUEST_MSK = /
s/DSP_SIG dsp_signal_lookup\[NUM_SIGNALS\] = {/SIGNAL_LOOKUP = [/
s/,/",/
s/""/"/
s/MAKE_DSP_SIG_ENTRY (/[0, 99, 1, "/
s/MAKE_DSP_SIG_ENTRY_VECTOR (\(*\), /[0, 99, \1, /
s/MAKE_DSP_SIG_ENTRY_END (/[-1, 0, 0, "no signal", /
s/"), /"], /
s/)  \/\/ END/, 0]  ## END/
s/};/]/
s/\/\//##/
s/#ifndef __SRANGER_MK3_DSP_SIGNALS_H//
s/#define __SRANGER_MK3_DSP_SIGNALS_H//
s/# ifdef CREATE_DSP_SIGNAL_LOOKUP//
s/# endif//
s/#endif//
s/#define DSP32Qs15dot16TOV  /DSP32Qs15dot16TOV  = /
s/#define DSP32Qs15dot16TO_Volt /DSP32Qs15dot16TO_Volt = /
s/#define DSP32Qs23dot8TO_Volt  /DSP32Qs23dot8TO_Volt  = /
s/#define DSP32Qs15dot0TO_Volt  /DSP32Qs15dot0TO_Volt  = /
s/#define NUM_SIGNALS/NUM_SIGNALS = /
s/#define CPN(N) ((double)(1LL<<(N))-1.)/\ndef CPN(N):\n        return ((double)(1LL<<(N))-1.)\n/
s/#pragma DATA_SECTION(dsp_signal_lookup", "SMAGIC")//
s/M_PI/math.pi/
s/(double)//
s/1LL/1/
s:/\*:#/\*:
s/#define //
s/FB_SPM_FLASHBLOCK_IDENTIFICATION_A/FB_SPM_FLASHBLOCK_XIDENTIFICATION_A = /
s/FB_SPM_FLASHBLOCK_IDENTIFICATION_B/FB_SPM_FLASHBLOCK_XIDENTIFICATION_B = /
s/_ID /_ID = /
s/_ERROR/_ERROR = /
s/_MISMATCH /_MISMATCH = /
s/_WARNING /_WARNING = /
s/_VERSION /_VERSION = /
s/_LIMIT /_LIMIT = /
