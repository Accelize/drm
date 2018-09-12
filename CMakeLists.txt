# Copyright (C) 2018, Accelize
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required (VERSION 2.8.12)

project(accelize/drmlib)

# Get version
execute_process(COMMAND git describe --tag --dirty=-d
                COMMAND sed s/^v//
                COMMAND sed s/-/_/g
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                OUTPUT_VARIABLE GIT_VERSION
                OUTPUT_STRIP_TRAILING_WHITESPACE)
message(STATUS "Library version from Git : ${GIT_VERSION}")
string(REPLACE "." ";" VERSION_LIST ${GIT_VERSION})
list(GET VERSION_LIST 0 VERSION_MAJOR)
set(ABI_VERSION ${VERSION_MAJOR})
message(STATUS "ABI library version from Git : ${ABI_VERSION}")

add_compile_options(-std=c++11)
add_compile_options(-Wall -Wextra -Wnon-virtual-dtor -pedantic)
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

find_package(Threads REQUIRED)
find_package(CURL REQUIRED)
find_package(jsoncpp REQUIRED)

# Generate public headers
execute_process(COMMAND uname -r OUTPUT_VARIABLE UNAME_R OUTPUT_STRIP_TRAILING_WHITESPACE)
find_program(UNIFDEF_BIN unifdef PATHS /usr/src/kernels/${UNAME_R}/scripts)
if(NOT UNIFDEF_BIN)
    message(FATAL_ERROR "unifdef not found")
endif()
set(API_HEADERS
  include/accelize/drm/session_manager.h
  include/accelize/drm/error.h
  include/accelize/drm/metering.h
  include/accelize/drm.h
  include/accelize/drmc/errorcode.h
  include/accelize/drmc/metering.h
  include/accelize/drmc/version.h
  include/accelize/drmc.h
)
set(PUBLIC_HEADERS_OPTIONS "-UPRIV_HEADER")

set(API_PUBLIC_HEADERS)
foreach(_header_file ${API_HEADERS})
  get_filename_component(_header_file_path ${_header_file} DIRECTORY)
  add_custom_command(OUTPUT ${_header_file}
                     COMMAND mkdir -p ${_header_file_path}
                     COMMAND ${UNIFDEF_BIN} ${PUBLIC_HEADERS_OPTIONS} ${CMAKE_SOURCE_DIR}/${_header_file} > ${_header_file} || [ $$? -eq 0 ] || [ $$? -eq 1 ]
                     DEPENDS ${CMAKE_SOURCE_DIR}/${_header_file})
  list(APPEND API_PUBLIC_HEADERS ${CMAKE_BINARY_DIR}/${_header_file})
endforeach()
configure_file(include/accelize/drm/version.h.in include/accelize/drm/version.h)
list(APPEND API_PUBLIC_HEADERS ${CMAKE_BINARY_DIR}/include/accelize/drm/version.h)
add_custom_target(public_headers ALL DEPENDS ${API_PUBLIC_HEADERS})

set(TARGET_SOURCES
  source/ws_client.cpp
  source/session_manager.cpp
  source/utils.cpp
  source/version.cpp
  source/error.cpp
)

set(TARGET_C_SOURCES
  source/c/wrapperc.cpp
)

include_directories(${CMAKE_BINARY_DIR}/include)
include_directories(internal_inc)
include_directories(include)
include_directories(${CURL_INCLUDE_DIR})
include_directories(drm_controller_sdk/drm_controller_sdk/include)

# DRM_CONTROLLER_SDK
set(DRM_CONTROLLER_SDK_SOURCES
  drm_controller_sdk/drm_controller_sdk/source/DrmControllerCommon.cpp
  drm_controller_sdk/drm_controller_sdk/source/DrmControllerDataConverter.cpp
  drm_controller_sdk/drm_controller_sdk/source/DrmControllerVersion.cpp
  drm_controller_sdk/drm_controller_sdk/source/HAL/DrmControllerOperations.cpp
  drm_controller_sdk/drm_controller_sdk/source/HAL/DrmControllerRegisters.cpp
)
set_source_files_properties(${DRM_CONTROLLER_SDK_SOURCES} PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter")
add_library(drm_controller_lib STATIC ${DRM_CONTROLLER_SDK_SOURCES})
set_target_properties(drm_controller_lib PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_options(drm_controller_lib PRIVATE -DVERSION_CHECK_DISABLED)

add_library(accelize_drm SHARED ${TARGET_SOURCES})
set_target_properties(accelize_drm PROPERTIES VERSION ${GIT_VERSION} SOVERSION ${ABI_VERSION})
target_link_libraries(accelize_drm ${CURL_LIBRARIES})
target_link_libraries(accelize_drm jsoncpp)
target_link_libraries(accelize_drm drm_controller_lib)
target_link_libraries(accelize_drm ${CMAKE_THREAD_LIBS_INIT})

add_library(accelize_drmc SHARED ${TARGET_C_SOURCES})
set_target_properties(accelize_drmc PROPERTIES VERSION ${GIT_VERSION} SOVERSION ${ABI_VERSION})
target_link_libraries(accelize_drmc accelize_drm)

# Install
include(GNUInstallDirs)
install(TARGETS accelize_drm LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT libraries)
install(TARGETS accelize_drmc LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT libraries)
install(DIRECTORY ${CMAKE_BINARY_DIR}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} COMPONENT headers)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/licenses/ DESTINATION  ${CMAKE_INSTALL_DOCDIR}/licenses/ COMPONENT licenses)

# Documentation
option(DOC "Produce documentation" OFF)
if(DOC)
  find_package(Doxygen REQUIRED)
  string(REPLACE ";" " " API_PUBLIC_HEADERS_DOC "${API_PUBLIC_HEADERS}")
  configure_file(doc/Doxyfile.in doc/Doxyfile)
  add_custom_target(
    doc ALL
    COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile > /dev/null
    DEPENDS ${API_PUBLIC_HEADERS}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/doc
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM
  )
  install(DIRECTORY ${CMAKE_BINARY_DIR}/doc/html/ DESTINATION ${CMAKE_INSTALL_DOCDIR}/html/ COMPONENT doc)
endif(DOC)

# Packages
set(CPACK_GENERATOR "TGZ;RPM")
set(CPACK_PACKAGE_NAME "libaccelize_drm")
set(CPACK_PACKAGE_VENDOR "Accelize")
set(CPACK_PACKAGE_CONTACT "accelize.com")
set(CPACK_PACKAGE_MAINTAINER "accelize.com")
set(CPACK_DESCRIPTION_SUMMARY "libaccelize_drm - Library with CPP and C API for Accelize DRM Solution")
set(CPACK_PACKAGE_VERSION ${GIT_VERSION})
set(CPACK_PACKAGE_RELEASE 1)
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-el7.${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_RPM_CHANGELOG_FILE ${CMAKE_CURRENT_SOURCE_DIR}/CHANGELOG)

# RPM Packages
set(CPACK_RPM_COMPONENT_INSTALL ON)

include(CPack)

# Component groups
cpack_add_component_group(devel)

cpack_add_component(libraries GROUP devel)
cpack_add_component(headers GROUP devel)
cpack_add_component(doc GROUP devel)
cpack_add_component(licenses GROUP devel)
