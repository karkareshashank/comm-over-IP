#!/bin/bash

cd kernel_modules
make clean
make
sudo insmod cse536_protocol.ko
sudo insmod comm_device.ko
cd ../user_app
gcc cse536_app.c
sudo ./a.out
cd ../
