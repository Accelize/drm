#!/bin/bash

agfi=

while [[ $# -gt 1 ]]
do
key="$1"

case $key in
    -j|--json)
    agfi=$(cat $2 | grep agfi | cut -d'"' -f4)
    shift # past argument
    shift # past value
    ;;
    -a|--agfi)
    agfi=$2
    shift # past argument
    shift # past value
    ;;
    --default)
    DEFAULT=YES
    shift # past argument
    ;;
    *)    # unknown option
    shift # past argument
    ;;
esac
done

if [[ -z $agfi ]]; then
    echo "Bad usage!"
    echo "Usage: ./progFPGA.sh [-a|--agfi agfi_num] [-j|--json json_path]"
    echo "    -a|--agfi    AGFI number: agfi-xxxxxxxxxxxx"
    echo "    -j|--json    Path to json file containing the output of the AWS CLI command create-fpga-image"
    exit -1
fi

echo "Programming FPGA with $agfi"
sudo fpga-clear-local-image -S 0 -H
sudo fpga-load-local-image -S 0 -I $agfi
sudo fpga-describe-local-image -S 0 -R -H
