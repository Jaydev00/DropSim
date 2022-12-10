#!/bin/bash
Procs=$(nproc)
#Procs=14
#if [ $Procs -gt 1 ]; then
#    Procs=$((Procs-1))
#fi
echo $Procs
./simulation -t $Procs -s 100000 -r 1/100 -u 10 -p 5 -l 3 -c 10000
