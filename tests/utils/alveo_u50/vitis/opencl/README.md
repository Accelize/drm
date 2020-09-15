bitstream:
----------
xclbin folder contain a bitstream for u50 with DSA xilinx_u50_gen3x16_xdma_201920_3 and XRT v2.5


To compile:
-----------
source /opt/xilinx/xrt/setup.sh
make app

To run:
-------
sudo su
source /opt/xilinx/xrt/setup.sh
./host.exe xclbin/adder.xclbin

TEST PASSED is displayed if the application runs successfully.