#!/bin/bash
if [ $# -ne 2 ]
then 
	echo "usage: $0 <lcl_nodeid> <dcid>"
	exit 1 
fi
lcl=$1
let rmt=(1 - $lcl)
dcid=$2
echo "lcl_nodeid=$lcl dcid=$dcid" 
read  -p "Enter para continuar... "
dmesg -c > /dev/null
read  -p "Spread Enter para continuar... "
/usr/local/sbin/spread  > spread.txt &	
cd /usr/src/dvs/dvk-mod
mknod /dev/dvk c 33 0
dmesg -c > /usr/src/dvs/dvk-tests/dmesg.txt
insmod dvk.ko dvk_major=33 dvk_minor=0 dvk_nr_devs=1	
dmesg -c > /usr/src/dvs/dvk-tests/dmesg.txt
cd /usr/src/dvs/dvk-apps
./dvsd $lcl		
exit

