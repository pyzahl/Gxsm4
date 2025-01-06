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
