# **ATTENTION:**
![gxsm4-rp-icon](https://github.com/user-attachments/assets/3965aa8b-5241-44a0-87de-178a100f6803)

RedPitaya SPM-Control-PACPLL-Pulse Project

Experimental and development code only

(C) PyZahl 2025-01-05

# 1. General Information

GXSM -- Gnome X Scanning Microscopy: A multi-channel image and vector-probe data acquisition and visualization system designed for SPM techniques (STM,AFM..). A plug-in interface allows any user add-on data-processing and special hardware and instrument support.

This is a experimental complete all on one FPGA level SPM control.

All latest stable software is available now via GIT:
(Gxsm4 Beta currently) https://github.com/pyzahl/Gxsm4

or Live Demo/Install CD (with stable GXSM3):
http://www.ventiotec.de/linux/GXSM-Linux.iso

GXSM Web Site: http://gxsm.sf.net

# 2. Installation
You will need to grab the free version of Vivado from AMD / Xilinx. I am using 2023.1, but newer shall do also.

vivado -source make_all.tcl

... and go get a coffee ...
Notes: you may need to fix/adjust some path or mainly remove not needed lines w "missing" files, those are NOT required to rebuild. Those which may cause errors while sourcing... I am not going to edit this everytime I export it for rev control.

see also: https://github.com/jhallen/vivado_setup

# 3. Re-Create/Update project rebuild script:

This will recreate the project rebuild script and included all files to be added to the project, blocks, schematics and netlist, ...
Very important if creating new files of any kind to NEVER designate or use the default in-project-folder destination path! 
But choose a approproiate folder from "rtl", "sim", "cfg" or even "cores" -- or if needed create a new folder outside the project.
Set the "root" (or start vivado form there) "here" where this README.md file is located!

Vivado tcl command:

write_project_tcl recreated-new-version.tcl

to update/over write:
write_project_tcl -force recreate-rpspmc-pacpll-vivado-project.tcl


# 4. A global design overview at Block and Interface Level:

![Screenshot from 2025-01-06 12-41-03](https://github.com/user-attachments/assets/6ca60d98-c22e-41b5-a974-3a3a0460623a)

PAC-PLL Block:
![Screenshot from 2025-01-06 12-53-27](https://github.com/user-attachments/assets/da044cf5-2aad-40fc-b1a9-e925d70069a6)

SPMC Main Block:
![Screenshot from 2025-01-06 13-05-21](https://github.com/user-attachments/assets/4fdd3691-850d-4284-9ffc-14a9efa70e19)

Z-Servo Controller Block:
![Screenshot from 2025-01-06 13-09-56](https://github.com/user-attachments/assets/cc823f36-4cd9-4d33-bbfb-c968eb557e47)

# 5. Hardware:
A analog IO adopting hat for the RP was designed.

![rpZ7020_adda_io_shield-withRP-pmod-eval2](https://github.com/user-attachments/assets/8de8d4c9-3c87-4010-bda2-9541b2274633)

![rpZ7020_adda_io_shield-withRP-box](https://github.com/user-attachments/assets/75f21c69-2a45-4e50-a219-35888e172a63)

![Screenshot from 2025-03-09 11-12-35](https://github.com/user-attachments/assets/44f29106-87fb-4d10-8ead-d407623d7526)


PCBWay Link to RPSPMC hat rev 1.0.1: https://www.pcbway.com/project/shareproject/GXSM4_RPSPMC_hat_Rev1_0_1_199b9ff2.html



