#!/usr/bin/env bash

git clone https://github.com/aws/aws-fpga /dev/shm/aws-fpga --depth 1
source /dev/shm/aws-fpga/sdk_setup.sh
rm -Rf /dev/shm/aws-fpga
