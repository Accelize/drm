
set(DRMLIB_FOUND false)

if(NOT DEFINED ENV{DRMLIB_DIR})
  message(STATUS "Searching for DRMLib")

  find_path(DRMLIB_INCLUDE_DIRS
	  NAMES accelize/drm.h
	  )

  find_library(DRMLIB_LIBRARIES
	  NAMES accelize_drm
	  )

  find_library(DRMLIB_C_LIBRARIES
	  NAMES accelize_drmc
	  )

else()
  message(STATUS "Searching for DRMLib using $ENV{DRMLIB_DIR}")

  find_path(DRMLIB_INCLUDE_DIRS
	  NAMES accelize/drm.h
	  PATHS $ENV{DRMLIB_DIR}/include/
	  NO_DEFAULT_PATH
	  )

  find_library(DRMLIB_LIBRARIES
	  NAMES accelize_drm
	  PATHS $ENV{DRMLIB_DIR}
	  $ENV{DRMLIB_DIR}/lib
	  $ENV{DRMLIB_DIR}/build
	  NO_DEFAULT_PATH
	  )

  find_library(DRMLIB_C_LIBRARIES
	  NAMES accelize_drmc
	  PATHS $ENV{DRMLIB_DIR}
	  $ENV{DRMLIB_DIR}/lib
	  $ENV{DRMLIB_DIR}/build
	  NO_DEFAULT_PATH
	  )

endif()



if(DRMLIB_LIBRARIES)
  message(STATUS "DRMLib Found lib : ${DRMLIB_LIBRARIES}")
endif()

if(DRMLIB_C_LIBRARIES)
  message(STATUS "DRMLib-C Found lib : ${DRMLIB_C_LIBRARIES}")
endif()

if(DRMLIB_INCLUDE_DIRS)
  message(STATUS "DRMLib Found includes : ${DRMLIB_INCLUDE_DIRS}")
endif()

if(DRMLIB_LIBRARIES AND DRMLIB_C_LIBRARIES AND DRMLIB_INCLUDE_DIRS)
  set(DRMLIB_FOUND true)
endif()

if(${DRMLIB_FIND_REQUIRED} AND NOT DRMLIB_FOUND)
  message(FATAL_ERROR "DRMLib not Found, please try to set DRMLIB_DIR to point to build directory")
endif()
