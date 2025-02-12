# ** ATTENTION ** VERY EXPERIMENTAL **

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

1. Build the FPGA bitstream "system_wrapper.bit" using Vivado as of instructions in the parent folder.
2. UPDATE: for the latest 2.0 RedPitaya EcoSystem the Vivado output system_wrapper.bit file has to be converted into a .bit.bin file using the provided script "convert_cpy_fpga_system.sh". You will have to edit and adjust teh destination RP's IP address for install via ssh/scp. Also the script does require the conversion tool from Xilinx/AMD what comes with Vivado and can be found (adjust path) in the according "Xilinx/Vivad/bin" location.


4. Copy/Link the resulting implemenation/FPGA code here to fpga.bit (please adjust "project_RP-SPMC-RedPACPLL-202308-test/project_RP-SPMC-RedPACPLL-202308-test" to your current project folder name):

$ ln -s ../project_RP-SPMC-RedPACPLL-202308-test/project_RP-SPMC-RedPACPLL-202308-test.runs/impl_1/system_wrapper.bit fpga.bit

3. ssh into your RedPitaya (adjust the RP's hostname rp-f05603.local to yours) and leve the terminal open:

$ ssh root@rp-f05603.local
 
Then make your RP's filesystem read/write, issue command:

$ rw

4. Copy this folder "rpspmc" (do a cd ..) recursively to your RedPitaya (adjust the RP's hostname rp-f05603.local to yours) -- the current build is for a 7Z020:

$ scp -r rpspmc root@rp-f05603.local:/opt/redpitaya/www/apps/

5. in your RP's terminal change to the folder rpspmc just copied over:

$ cd /opt/redpitaya/www/apps/rpspmc

6. build the Zynq internet json bridge fro you RPSPMC:

$ make clean; make INSTALL_DIR=/opt/redpitaya

7. Check for connection to your RP in a web browser and see if "RPSPMC" app is showing. NOTE: There is currently no functional (old dummy/test hack) WEB FRONTEND, GXSM4 is to be used!

