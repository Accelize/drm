# Use this script to install the dependencies required to execute the non-regression system.
# Must be executed with sudo

# Install dependencies
#If gcc/g++ is not installed by default: dnf install -y packagegroup-petalinux-self-hosted
dnf install -y git curl-dev libjsoncpp-dev

# Install Python dependencies
pip3 install backports.entry-points-selectable filelock
pip3 install wheel pyopencl pytest pytest-flask flake8

# Compile DRM library
#If drm folder is not already installed: git clone --recursive https://github.com/Accelize/drm.git -b kria
#cd drm
mkdir build
cd build
cmake -DPYTHON3=ON -DTESTS=ON -DCMAKE_BUILD_TYPE=Debug ..
make
DRMLIB_BUILD=$(pwd)
cd ..

# Before executing the non-regression script
export XILINX_XRT=/usr

# To execute the non-regression script in C
LD_LIBRARY_PATH=$DRMLIB_BUILD python3 -m pytest --backend=c --cred=../../cred.json --server=dev --fpga_image=fpga-drm-hybrid-adder.som --drm_controller_base_address=0xA0010000 -ra tests
# To execute the non-regression script in C++
LD_LIBRARY_PATH=$DRMLIB_BUILD python3 -m pytest --backend=c++ --cred=../../cred.json --server=dev --fpga_image=fpga-drm-hybrid-adder.som --drm_controller_base_address=0xA0010000 -ra tests
