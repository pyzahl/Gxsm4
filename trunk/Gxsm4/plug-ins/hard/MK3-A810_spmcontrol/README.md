
# GXSM 3.0
## MK3-PLL firmware
There are different firmware files for softdb's  open source SPM controller model MK3-PLL available. Please select the one according to your needs. If you are unsure, please, visit the respective forum at gxsm.sf.net. To upload the firmware to the DSP, please use one of the following tools supplied by softdb:
1. SRx_Analog_Selftest + SPMPLL_reset
2. minidebugger
3. ...

To do so you will need a windows PC and install the following programs
https://www.softdb.com/_files/_dsp_division/SR3_Applications_Installer_V212.zip
https://www.softdb.com/_files/_dsp_division/SoftdB_SPM_PLL_1_84b.exe
https://www.softdb.com/_files/_dsp_division/SR3_Driver_Install_V300.exe

If you have trouble to re-enable the PLL algorithm on your controller, please contact softdb directly. They will have to add your controller to the list included in SPMPLL_reset.
 
### FB_spmcontrol-DEFAULT.out
* should be used for any cases when the DSP PAC-PLL is not used or not critical
* full scan speed, scan rotation, input filtering, ...

### FB_spmcontrol-MIN-PAC.out
* optimized for PAC-PLL
* non-essential DSP functionalities as additional current input filter processing are stripped to free DSP time 
* high priority to phase alignments, but phase glitches can occasionally happen
* DSP performance and therefore scan speed is reduced!
* should not be used as default

### FB_spmcontrol-MIN-PAC-NOROT.out  
* frees up even more DSP time
* no scan rotation any more possible (as this is related to a fairly costly DSP matrix multiplication at 32/64 bit precision

### FB_spmcontrol.out
This is simple the last build (by Percy Zahl). It should but has not to be identical with DEFAULT!

## History
### V1.0 11.07.2021 by stm
initial version of the document
 
