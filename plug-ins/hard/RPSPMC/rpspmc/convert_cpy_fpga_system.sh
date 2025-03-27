#!/bin/sh

echo
echo "  ************************************************************"
echo "  *** RedPitaya Ecosystem auto update/install script"
echo "  ************************************************************"
echo "  ***"
echo "  ***              requirements for auto install RP:"
echo "  ***                 sshpass, configured script (this)"
echo "  ***"
echo "  Converting bit file and installing from '"`hostname`"' on RP -- please make sure your RP's FS is in rw mode:"
echo "  ***"
#read -p "  Do you want to do a full install? (y/n) " fyn
read -p "  Do you want to copy the source files in src/*.cpp, *.h also over? (y/n) " yn
echo "  ***"
echo
cp ../project_RP-SPMC-PACPLL-Gxsm4/project_RP-SPMC-PACPLL-Gxsm4.runs/impl_1/system_wrapper.bit fpga.bit
echo -n "all:{ ./fpga.bit }" >  fpga.bif
echo

# PLEASE CONFIGURE you RP's IP below

case `hostname` in
    (phenom)
    	echo "on phenom: generating bit.bin:";
	/mnt/backupC/Xilinx23/Vivado/2023.1/bin/bootgen -image fpga.bif -arch zynq -process_bitstream bin -w -o fpga.bit.bin;
	#rp = "rp-f09296.local"   ## RP's IP, dynamic, very slow resolving
	rp="192.168.0.7";  ## RP's IP
	rpp="root"
    	echo "copy to RP root@rp-f09296.local == root@"$rp;
	break;;

    (bititan.cfn.bnl.gov) 
	echo "on bititan: generating bit.bin:";
	/mnt/home-copy/Xilinx2023/Vivado/2023.1/bin/bootgen -image fpga.bif -arch zynq -process_bitstream bin -w -o fpga.bit.bin;
	rp="130.199.243.110";   ## RP's IP
	rpp="root4gxsm"
	break;;
    (your.gxsmhost.uni.net) ### TEMPLATE: COPY TO NEW CASE ITEM and UPDATE!
	echo "on xyz host: generating bit.bin:";
	/your-path-to/Vivado/2023.1/bin/bootgen -image fpga.bif -arch zynq -process_bitstream bin -w -o fpga.bit.bin;
	#rp = "rp-f09296.local" ## RP's IP, dynamic, very slow resolving
	rp="130.199.243.110";   ## your RP's IP, resolved / static number (fast)
	rpp="root"   ## your RP's ssh password
	break;;
    (*)   echo "Please edit me and adjust the path for bootgen and your RP's IP for you system '"`hostname`"'..."; exit;;
esac

echo
echo "  *** Attempting making RP FS rw ***"
sshpass -p $rpp ssh root@$rp "mount -o rw,remount /opt/redpitaya/"
echo
echo "  *** copy to RP root@"$rp;
sshpass -p $rpp scp fpga.bit.bin root@$rp:/opt/redpitaya/www/apps/rpspmc; 
case $yn in
    [Yy]* ) echo "Copying sources..."; 
    	    sshpass -p $rpp scp src/*.h src/*.cpp root@$rp:/opt/redpitaya/www/apps/rpspmc/src;
	    echo
	    echo "  ***"
	    echo "  Now run:   make clean; make";
	    echo "  ...  in /opt/redpitaya/www/apps/rpspmc/"
	    echo "  ...  on your RP to rebuild & update your RP's RPSPMC server.";
	    echo "  ***"
	    echo "  *** Attempting to remote build/install ***"
	    echo
	    sshpass -p $rpp ssh root@$rp "cd /opt/redpitaya/www/apps/rpspmc/; make clean; make"
	    echo
	    echo "  *** Update/install completed if no errors above. ***"
	    break;;
esac

echo
echo "  Done copying."
echo

