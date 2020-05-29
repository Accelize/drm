
set(FPGAMGMTLIB_FOUND false)

message(STATUS "Searching for FpgaMgmt Lib using ${FPGAMGMTLIB_DIR}")

find_path(FPGAMGMTLIB_INCLUDE_DIRS
  NAMES fpga_mgmt.h
  PATHS ${FPGAMGMTLIB_DIR}/userspace/include $ENV{SDK_DIR}/userspace/include /usr/local/include NO_DEFAULT_PATH
)

find_library(FPGAMGMTLIB_LIBRARIES
  NAMES fpga_mgmt
  PATHS ${FPGAMGMTLIB_DIR}/userspace/lib/so $ENV{SDK_DIR}/userspace/lib/so /usr/local/lib64 NO_DEFAULT_PATH
)

if(FPGAMGMTLIB_LIBRARIES)
  message(STATUS "FpgaMgmtLib Found lib : ${FPGAMGMTLIB_LIBRARIES}")
endif()

if(FPGAMGMTLIB_INCLUDE_DIRS)
  message(STATUS "FpgaMgmtLib Found includes : ${FPGAMGMTLIB_INCLUDE_DIRS}")
endif()

if(FPGAMGMTLIB_LIBRARIES AND FPGAMGMTLIB_INCLUDE_DIRS)
  set(FPGAMGMTLIB_FOUND true)
endif()

if(${FPGAMGMTLIB_FIND_REQUIRED} AND NOT FPGAMGMTLIB_FOUND)
  message(FATAL_ERROR "FpgaMgmtLib not Found, please try to set FPGAMGMTLIB_DIR to point to build directory")
endif()
