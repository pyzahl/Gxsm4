#!/bin/sh

MODEL=$(/opt/redpitaya/bin/monitor -f)

rmdir /configfs/device-tree/overlays/Full 2> /dev/null
rm /tmp/loaded_fpga.inf 2> /dev/null

sleep 0.5s

echo "Loading RPSPMC FPGA.bit"
/opt/redpitaya/bin/fpgautil -b /opt/redpitaya/www/apps/rpspmc/fpga.bit.bin

rm /tmp/loaded_fpga.inf 2> /dev/null

exit 1
