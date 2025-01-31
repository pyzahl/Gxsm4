#!/bin/sh

echo "Converting bit file and installing..."

cp ../project_RP-SPMC-PACPLL-Gxsm4/project_RP-SPMC-PACPLL-Gxsm4.runs/impl_1/system_wrapper.bit fpga.bit

echo -n "all:{ ./fpga.bit }" >  fpga.bif

case `hostname` in
    (phenom)
	echo "on phenom: generating bit.bin:";
	/mnt/backupC/Xilinx23/Vivado/2023.1/bin/bootgen -image fpga.bif -arch zynq -process_bitstream bin -w -o fpga.bit.bin;
	echo "copy to RP root@rp-f09296.local";
	scp fpga.bit.bin  root@rp-f09296.local:/opt/redpitaya/www/apps/rpspmc/;;
    (bititan) 
	echo "on bititan: generating bit.bin:";
	/mnt/home-copy/Xilinx2023/Vivado/2023.1/bin/bootgen -image fpga.bif -arch zynq -process_bitstream bin -w -o fpga.bit.bin;
	echo "copy to RP root@130.199.243.110";
	scp fpga.bit.bin  root@130.199.243.110:/opt/redpitaya/www/apps/rpspmc/;;
    (*)   echo "Adjust for your path...";;
esac

echo "Done."
