#!/bin/bash
if [ $# -ne 1 ]
then 
	echo "usage: $0 <local_nodeid>"
	exit 1 
fi
lcl=$1
echo "local_nodeid=$lcl" 
read  -p "Enter para continuar... "
dmesg -c > /dev/null
# cd /home/MoL_Module/mol-module
insmod /home/MoL_Module/mol-module/mol_replace.ko
lsmod
read  -p "Spread Enter para continuar... "
/usr/local/sbin/spread  > spread.txt &	
./dvsd $lcl		
exit

