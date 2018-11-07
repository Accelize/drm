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

if(COVERAGE)
include(release-tools/cmake/codecoverage.cmake)
endif()

# Build Type
set(default_build_type "RelWithDebInfo")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

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
add_compile_options(-fvisibility=hidden)
add_compile_options(-Wall -Wextra -Wnon-virtual-dtor -pedantic)
add_compile_options("$<$<CONFIG:DEBUG>:-Og>")
add_compile_options(-fdebug-prefix-map=${CMAKE_SOURCE_DIR}=.)
string(LENGTH "${CMAKE_SOURCE_DIR}/" SOURCE_PATH_SIZE)
add_definitions("-DSOURCE_PATH_SIZE=${SOURCE_PATH_SIZE}")
add_definitions(-DBUILDING_DRMLIB)
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
  include/accelize/drmc/common.h
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
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE DESTINATION  ${CMAKE_INSTALL_DOCDIR}/licenses COMPONENT licenses)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/third-party-licenses/ DESTINATION  ${CMAKE_INSTALL_DOCDIR}/licenses/third-party-licenses/ COMPONENT licenses)

message("CMAKE_BINARY_DIR is ${CMAKE_BINARY_DIR}")
message("CMAKE_CURRENT_SOURCE_DIR is ${CMAKE_CURRENT_SOURCE_DIR}")

# Code coverage
option(COVERAGE "Activate code coverage" OFF)
if(COVERAGE)
  get_filename_component(_coverage_path ${CMAKE_BINARY_DIR} DIRECTORY)

  APPEND_COVERAGE_FLAGS()
  # Install coverage information (.gcno) and objects files generated by compiler
  install(DIRECTORY ${CMAKE_BINARY_DIR}/CMakeFiles DESTINATION build_coverage/build/${CMAKE_BINARY_DIR}/ COMPONENT coverage)

  # Install sources
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/drm_controller_sdk DESTINATION build_coverage/src/${CMAKE_CURRENT_SOURCE_DIR}/ COMPONENT coverage)
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include DESTINATION build_coverage/src/${CMAKE_CURRENT_SOURCE_DIR}/ COMPONENT coverage)
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/internal_inc DESTINATION build_coverage/src/${CMAKE_CURRENT_SOURCE_DIR}/ COMPONENT coverage)
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/source DESTINATION build_coverage/src/${CMAKE_CURRENT_SOURCE_DIR}/ COMPONENT coverage)
endif(COVERAGE)

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
set(CPACK_PACKAGE_NAME "libaccelize_drm${PACKAGE_NAME_SUFFIX}")
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

if(COVERAGE)
cpack_add_component(coverage GROUP devel)
endif()
