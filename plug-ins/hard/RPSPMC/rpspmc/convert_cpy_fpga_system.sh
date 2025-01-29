#!/bin/sh

echo "Sorry for this been required, please adopt paths below"

cp ../project_RP-SPMC-PACPLL-Gxsm4/project_RP-SPMC-PACPLL-Gxsm4.runs/impl_1/system_wrapper.bit fpga.bit

echo -n "all:{ ./fpga.bit }" >  fpga.bif

/mnt/backupC/Xilinx23/Vivado/2023.1/bin/bootgen -image fpga.bif -arch zynq -process_bitstream bin -o fpga.bit.bin
#/mnt/home-copy/Xilinx2023/Vivado/2023.1/bin/bootgen -image fpga.bif -arch zynq -process_bitstream bin -o fpga.bit.bin

scp fpga.bit.bin  root@rp-f09296.local:/opt/redpitaya/www/apps/rpspmc/

