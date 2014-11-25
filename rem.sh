#!/bin/bash

sudo rmmod comm_device
sudo rmmod cse536_protocol
cd kernel_modules
make clean
cd ../
rm ./user_app/a.out
