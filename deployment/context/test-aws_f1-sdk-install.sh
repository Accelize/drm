#!/usr/bin/env bash

git clone https://github.com/aws/aws-fpga /tmp/aws-fpga --depth 1
source /tmp/aws-fpga/sdk_setup.sh
rm -Rf /tmp/aws-fpga
