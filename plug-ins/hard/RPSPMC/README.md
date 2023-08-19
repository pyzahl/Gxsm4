# **ATTENTION:**

RedPitaya SPM-Control-PACPLL-Pulse Project

Experimental and development code only

(C) PyZahl 2023-08-15

# 1. General Information

GXSM -- Gnome X Scanning Microscopy: A multi-channel image and vector-probe data acquisition and visualization system designed for SPM techniques (STM,AFM..). A plug-in interface allows any user add-on data-processing and special hardware and instrument support.

Complete FPGA level SPM control.

All latest stable software is available now via GIT:
(Gxsm4 Beta currently) https://github.com/pyzahl/Gxsm4

or Live Demo/Install CD (with GXSM3):
http://www.ventiotec.de/linux/GXSM-Linux.iso

GXSM Web Site: http://gxsm.sf.net

# 2. Installation

vivado -source make_all.tcl

... and go get a coffee ...

see also: https://github.com/jhallen/vivado_setup

# 3. Re-Create/Update project rebuild script:

This will recreate the project rebuld script and included all files to be added to the project, blocks, schematics and netlist, ...
Very important if creating new files of any kind to NEVER designate or use the default in-project-folder destination path! 
But choose a approproiate folder from "rtl", "sim", "cfg" or even "cores" -- or if needed create a new folder outside the project.
Set the "root" (or start vivado form there) "here" where this README.md file is located!

Vivado tcl command:
write_project_tcl recreated-new-version.tcl
