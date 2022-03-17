
set(XILINXLIB_FOUND false)

message(STATUS "Searching for Xilinx Lib using ${XILINXLIB_DIR}")

find_path(XILINXLIB_INCLUDE_DIRS
  NAMES xclhal2.h
  PATHS $ENV{XILINX_XRT}/include /usr/local/include NO_DEFAULT_PATH
)

find_library(XRTLIB_LIBRARIES
  NAMES xrt_core
  PATHS $ENV{XILINX_XRT}/lib /usr/local/lib64 NO_DEFAULT_PATH
)

find_library(XILINXLIB_LIBRARIES
  NAMES xilinxopencl
  PATHS $ENV{XILINX_XRT}/lib /usr/local/lib64 NO_DEFAULT_PATH
)

if(XILINXLIB_INCLUDE_DIRS)
  message(STATUS "Xilinx Lib Found includes : ${XILINXLIB_INCLUDE_DIRS}")
endif()

if(XILINXLIB_LIBRARIES AND XRTLIB_LIBRARIES)
  message(STATUS "Xilinx Lib Found lib : ${XILINXLIB_LIBRARIES} ${XRTLIB_LIBRARIES}")
endif()

if(XILINXLIB_INCLUDE_DIRS AND XILINXLIB_LIBRARIES AND XRTLIB_LIBRARIES)
  set(XILINXLIB_FOUND true)
endif()

if(${XILINXLIB_FIND_REQUIRED} AND NOT XILINXLIB_FOUND)
  message(FATAL_ERROR "Xilinx Lib not Found, please try to set XILINXLIB_DIR to point to build directory")
endif()
