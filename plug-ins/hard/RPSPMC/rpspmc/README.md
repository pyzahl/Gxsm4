# ** ATTENTION ** EXPERIMENTAL BETA VERSION **

RedPitaya SPM-Control-PACPLL-Pulse Project

Experimental and development code only

(C) PyZahl 2023-08-15 - 2025

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

A0) IN VIVADO's tcl console type (first time) "source make_all.tcl"   This creates a project folder "project_RP-SPMC-PACPLL-Gxsm4" and is setting up everything, takes a moment.
Then click on the "make Bitstream" Icon in Vivado and have a coffee... and let the majac happen.

A1) TO UPDATE, delete this project folder "project_RP-SPMC-PACPLL-Gxsm4" completely, then run from the Vivado's tcl console (close any project before, delete the folder!) only "source recreate-rpspmc-pacpll-vivado-project.tcl" and again click make Bitstream once all is imported. 
  
3. UPDATE OPN RP: for the latest 2.0 RedPitaya EcoSystem the Vivado output system_wrapper.bit file has to be converted into a .bit.bin file using the provided script "convert_cpy_fpga_system.sh". You will have to edit and adjust teh destination RP's IP address for install via ssh/scp. Also the script does require the conversion tool from Xilinx/AMD what comes with Vivado and can be found (adjust path) in the according "Xilinx/Vivad/bin" location.

B) COPY the "rpspmc" folder to you RP:

3. ssh into your RedPitaya (adjust the RP's hostname rp-f05603.local to yours) and leave the terminal open:

$ ssh root@rp-f05603.local
 
Then make your RP's filesystem read/write, issue command:

$ rw

4. Copy this folder "rpspmc" (do a cd ..) recursively to your RedPitaya (adjust the RP's hostname rp-f05603.local to yours) -- the current build is for a 7Z020:

$ scp -r rpspmc root@rp-f05603.local:/opt/redpitaya/www/apps/

4b. AND create a secondary copy for the "reconnect" operation mode, note the new destination "rpspmcrc":

$ scp -r rpspmc root@rp-f05603.local:/opt/redpitaya/www/apps/rpspmcrc

Please verify both folders been created on your RP in /opt/redpitaya/www/apps.

C) Now update and build your RP "RPSPMC" server:
The latest git included this script to automatize the RP update and install.
But you do need to customize this script "convert_cpy_fpga_system.sh" first, use you favorite editor and have a closer look, follow instructions/comments!
You will see a template section, copy and adjust: you need to add you machine name (the PC you use and the RP's IP and adjust the path to Vivado's bootgen tool) in this script, then it will all work automatically.

Once done, run it adn let the magic happen!

$$:~/SVN/Gxsm4/plug-ins/hard/RPSPMC/rpspmc$ ./convert_cpy_fpga_system.sh

  ************************************************************
  *** RedPitaya Ecosystem auto update/install script
  ************************************************************
  ***
  ***              requirements for auto install RP:
  ***                 sshpass, configured script (this)
  ***
  Converting bit file and installing from 'bititan.cfn.bnl.gov' on RP -- please make sure your RP's FS is in rw mode:
  ***
  Do you want to copy the source files in src/*.cpp, *.h also over? (y/n) y
  ***


on bititan: generating bit.bin:


****** Bootgen v2023.1.1
  **** Build date : May 12 2023-00:45:12
    ** Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
    ** Copyright 2022-2023 Advanced Micro Devices, Inc. All Rights Reserved.


[INFO]   : Bootimage generated successfully


  *** Attempting making RP FS rw ***

  *** copy to RP root@130.199.243.110
Copying sources...

  ***
  Now run:   make clean; make
  ...  in /opt/redpitaya/www/apps/rpspmc/
  ...  on your RP to rebuild & update your RP's RPSPMC server.
  ***
  *** Attempting to remote build/install ***

make -C src clean
make[1]: Entering directory '/opt/redpitaya/www/apps/rpspmc/src'
rm -f ../controllerhf.so main.o
make[1]: Leaving directory '/opt/redpitaya/www/apps/rpspmc/src'
rm -f *.so
make -C src
make[1]: Entering directory '/opt/redpitaya/www/apps/rpspmc/src'
g++ -c -fPIC -Os -s -Wno-reorder -Wno-deprecated -Wno-cpp -std=c++11 -I/opt/redpitaya//include -I/opt/redpitaya//include/api2 -I/opt/redpitaya//include/apiApp -I/opt/redpitaya//rp_sdk -I/opt/redpitaya//rp_sdk/libjson main.cpp -o main.o

.... some gcc output to watch ...

g++ main.o -o ../controllerhf.so -shared -fPIC -Os -s -Wno-reorder -Wno-deprecated -Wno-cpp -L/opt/redpitaya//lib -L/opt/redpitaya//rp_sdk -Wl,--whole-archive,--no-as-needed -lcryptopp -lrpapp -lrp -lrp_sdk -lrp-hw-calib -lrp-hw-profiles -Wl,--no-whole-archive
make[1]: Leaving directory '/opt/redpitaya/www/apps/rpspmc/src'


  *** Update/install completed successfully, copying controllerhf.so to ../rpspmcrc/

  Done copying.

$$:~/Gxsm4/plug-ins/hard/RPSPMC/rpspmc$ 


****************** OLD MANUAL VERSION *****************



4. Copy/Link the resulting implemenation/FPGA code here to fpga.bit (please adjust "project_RP-SPMC-RedPACPLL-202308-test/project_RP-SPMC-RedPACPLL-202308-test" to your current project folder name):

$ ln -s ../project_RP-SPMC-RedPACPLL-202308-test/project_RP-SPMC-RedPACPLL-202308-test.runs/impl_1/system_wrapper.bit fpga.bit

UPDATE: New RP Ecosystem requires bit.bin....

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

