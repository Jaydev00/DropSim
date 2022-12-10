#!/bin/bash
#  -a ./gained/EasyG.csv
Procs=$(nproc)
#Procs=1
./simulation -t $Procs -s 1000000 -r 3260/28080 -u 131 -p 3 -w Weights/EasyW.csv
