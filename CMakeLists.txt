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
cmake_minimum_required(VERSION 3.12)

# CONFIGURE VERSION AND COMMON INFORMATIONS

## Define package version
set(CPACK_PACKAGE_VERSION 2.0.0)

## Define package release number (Number of time this version was packaged)
if (NOT CPACK_PACKAGE_RELEASE)
    # Use 1 as default value if not defined with
    # "-DCPACK_PACKAGE_RELEASE=value" command line argument.
    set(CPACK_PACKAGE_RELEASE 1)
endif()

## Note: Python package version is synchronised with CPACK_PACKAGE_VERSION

## Configure common informations
set(CPACK_PACKAGE_VENDOR "Accelize")
set(CPACK_PACKAGE_CONTACT "https://www.accelize.com/contact")
set(CPACK_PACKAGE_MAINTAINER "accelize.com")
set(CPACK_DESCRIPTION_SUMMARY "Accelize DRM C and C++ Library")
set(CPACK_COMPONENT_DEV_DESCRIPTION "${CPACK_DESCRIPTION_SUMMARY}, headers files.")
set(CPACK_COMPONENT_PYTHON3_DESCRIPTION "Accelize DRM Python Library")
set(CMAKE_PROJECT_HOMEPAGE_URL https://github.com/Accelize/drmlib)
set(PROJECT_DOCUMENTATION_URL https://drmlib.readthedocs.io)
set(ACCELIZE_HOMEPAGE https://www.accelize.com)
set(ACCELIZE_EMAIL info@accelize.com)

## Find package CommitID
set(PACKAGE_LONG_VERSION ${CPACK_PACKAGE_VERSION})
find_program(GIT git)
if (GIT)
    execute_process(COMMAND git rev-parse --short HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_COMMIT
            OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    if (GIT_COMMIT)
        # Add as Semantic versioning build metadata
        set(PACKAGE_LONG_VERSION ${CPACK_PACKAGE_VERSION}+${GIT_COMMIT})
    endif ()
endif ()

## Find ABI version
string(REPLACE "." ";" VERSION_LIST ${CPACK_PACKAGE_VERSION})
list(GET VERSION_LIST 0 VERSION_MAJOR)
list(GET VERSION_LIST 1 VERSION_MINOR)
list(GET VERSION_LIST 2 VERSION_BUG)
set(ABI_VERSION ${VERSION_MAJOR})

## Show version information
message(STATUS "Library version : ${CPACK_PACKAGE_VERSION}")
message(STATUS "Library long version : ${PACKAGE_LONG_VERSION}")
message(STATUS "Library Package version : ${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}")

# CONFIGURE BUILD
project(accelize/drm)

## Build jobs
include(ProcessorCount)
ProcessorCount(CPU_COUNT)
set(CMAKE_BUILD_PARALLEL_LEVEL ${CPU_COUNT})

## Build type
set(default_build_type "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)

    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

## Compiler and linker options
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
set(CMAKE_CXX_STANDARD 11)
add_compile_options(-fvisibility=hidden)
if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    # Enable warnings
    add_compile_options(-O0 -Wall -Wextra -Wnon-virtual-dtor -Wpedantic)

    # Optimize for Debugging
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        add_compile_options(-fdebug-prefix-map=${CMAKE_SOURCE_DIR}=.)
    endif()

    # Code coverage
    option(COVERAGE "Activate code coverage" OFF)
    if(COVERAGE)
        # Add coverage compile and link options
        add_compile_options(--coverage)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
        if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
            link_libraries(gcov)
        endif()
    endif(COVERAGE)

else()
    # Optimize for runtime
    add_compile_options(-O3)
endif()

string(LENGTH "${CMAKE_SOURCE_DIR}/" SOURCE_PATH_SIZE)
add_definitions("-DSOURCE_PATH_SIZE=${SOURCE_PATH_SIZE}")
add_definitions(-DBUILDING_DRMLIB)
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")

## Build requirements
find_package(PkgConfig REQUIRED)

find_package(Threads REQUIRED)

pkg_check_modules(JSONCPP jsoncpp)
include_directories(${JSONCPP_INCLUDEDIR})

find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIR})

## drm_controller_sdk
set(DRM_CONTROLLER_SDK_SOURCES
        drm_controller_sdk/source/DrmControllerCommon.cpp
        drm_controller_sdk/source/DrmControllerDataConverter.cpp
        drm_controller_sdk/source/DrmControllerVersion.cpp
        drm_controller_sdk/source/HAL/DrmControllerOperations.cpp
        drm_controller_sdk/source/HAL/DrmControllerRegisters.cpp
        drm_controller_sdk/source/HAL/DrmControllerRegistersBase.cpp
        drm_controller_sdk/source/HAL/DrmControllerRegistersReport.cpp
        drm_controller_sdk/source/HAL/DrmControllerRegistersStrategyInterface.cpp
        drm_controller_sdk/source/HAL/DrmControllerRegistersStrategy_v3_0_0.cpp
        drm_controller_sdk/source/HAL/DrmControllerRegistersStrategy_v3_1_0.cpp
        drm_controller_sdk/source/HAL/DrmControllerRegistersStrategy_v3_2_0.cpp
)
include_directories(drm_controller_sdk/include)
set_source_files_properties(${DRM_CONTROLLER_SDK_SOURCES} PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter")
add_library(drm_controller_lib STATIC ${DRM_CONTROLLER_SDK_SOURCES})
set_target_properties(drm_controller_lib PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_options(drm_controller_lib PRIVATE -DVERSION_CHECK_DISABLED)

## Generate public headers
execute_process(COMMAND uname -r OUTPUT_VARIABLE UNAME_R OUTPUT_STRIP_TRAILING_WHITESPACE)

set(API_HEADERS
  include/accelize/drm/ParameterKey.def
  include/accelize/drm/session_manager.h
  include/accelize/drm/error.h
        include/accelize/drm.h
  include/accelize/drmc/common.h
  include/accelize/drmc/errorcode.h
        include/accelize/drmc/wrapper.h
  include/accelize/drmc.h
)

set(API_PUBLIC_HEADERS)
foreach(_header_file ${API_HEADERS})
  get_filename_component(_header_file_path ${_header_file} DIRECTORY)
  add_custom_command(OUTPUT ${_header_file}
                     COMMAND mkdir -p ${_header_file_path}
                     COMMAND cp ${CMAKE_SOURCE_DIR}/${_header_file} ${_header_file}
                     DEPENDS ${CMAKE_SOURCE_DIR}/${_header_file})
  list(APPEND API_PUBLIC_HEADERS ${CMAKE_BINARY_DIR}/${_header_file})
endforeach()
configure_file(include/accelize/drm/version.h.in include/accelize/drm/version.h)
list(APPEND API_PUBLIC_HEADERS ${CMAKE_BINARY_DIR}/include/accelize/drm/version.h)
add_custom_target(public_headers ALL DEPENDS ${API_PUBLIC_HEADERS})

set(CMAKE_INCLUDE_DIR ${CMAKE_BINARY_DIR}/include)
include_directories(${CMAKE_INCLUDE_DIR})
include_directories(internal_inc)
include_directories(include)

## libaccelize_drm
set(TARGET_SOURCES
  source/ws_client.cpp
  source/session_manager.cpp
  source/utils.cpp
  source/error.cpp
)

add_library(accelize_drm SHARED ${TARGET_SOURCES})
set_target_properties(accelize_drm PROPERTIES VERSION ${CPACK_PACKAGE_VERSION} SOVERSION ${ABI_VERSION})
target_link_libraries(accelize_drm ${CURL_LIBRARIES})
target_link_libraries(accelize_drm jsoncpp)
target_link_libraries(accelize_drm drm_controller_lib)
target_link_libraries(accelize_drm ${CMAKE_THREAD_LIBS_INIT})

## libaccelize_drmc
set(TARGET_C_SOURCES
    source/c/wrapperc.cpp
)

add_library(accelize_drmc SHARED ${TARGET_C_SOURCES})
set_target_properties(accelize_drmc PROPERTIES VERSION ${CPACK_PACKAGE_VERSION} SOVERSION ${ABI_VERSION})
target_link_libraries(accelize_drmc accelize_drm)

## Install and output directories
include(GNUInstallDirs)
install(TARGETS accelize_drm LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT libraries)
install(TARGETS accelize_drmc LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT libraries)
install(DIRECTORY ${CMAKE_INCLUDE_DIR}/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} COMPONENT headers)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/licenses/${PROJECT_NAME} COMPONENT licenses)

message(STATUS "Build dir : ${CMAKE_BINARY_DIR}")
message(STATUS "Source dir : ${CMAKE_CURRENT_SOURCE_DIR}")

set(CPACK_OUTPUT_FILE_PREFIX packages)
set(PACKAGE_OUTPUT ${CMAKE_BINARY_DIR}/${CPACK_OUTPUT_FILE_PREFIX})

# Python binding
option(PYTHON3 "Generates Python 3 library" OFF)
if (PYTHON3)
    include(FindPython3)

    ## Retrieves sources
    set(PYTHON3_BDIST_RELATIVE python3_bdist)
    set(PYTHON3_BDIST ${CMAKE_BINARY_DIR}/${PYTHON3_BDIST_RELATIVE})
    set(PYTHON_SOURCE ${CMAKE_BINARY_DIR}/python_src)
    set(PYTHON_SETUP ${PYTHON_SOURCE}/setup.py)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/python/ DESTINATION ${PYTHON_SOURCE}
            FILES_MATCHING PATTERN "*.in" PATTERN "*.py*" PATTERN "*.pxd" PATTERN "*.md")
    configure_file(${PYTHON_SOURCE}/accelize_drm/__init__.py
                   ${PYTHON_SOURCE}/accelize_drm/__init__.py)
    configure_file(${PYTHON_SETUP} ${PYTHON_SETUP})
    configure_file(${PYTHON_SOURCE}/README.md ${PYTHON_SOURCE}/README.md)

    ## Build and generate Wheel distribution + Source distribution
    if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        # Required for coverage (See "setup.py")
        set(BUILD_EXT_EXTRA --inplace)
    endif()

    add_custom_target(
            python3 ALL
            COMMAND
            ${Python3_EXECUTABLE} ${PYTHON_SETUP} bdist_wheel -b ${PYTHON3_BDIST} -d ${PACKAGE_OUTPUT} -k
                                                  build_ext -I ${CMAKE_INCLUDE_DIR}:${JSONCPP_INCLUDEDIR} -L ${CMAKE_BINARY_DIR} ${BUILD_EXT_EXTRA} -j ${CPU_COUNT} &&
            ${Python3_EXECUTABLE} ${PYTHON_SETUP} sdist -d ${PACKAGE_OUTPUT}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Building accelize_drm Python 3 Package"
            VERBATIM
    )
    add_dependencies(python3 accelize_drm)
    add_dependencies(python3 accelize_drmc)
    add_dependencies(python3 Python3::Interpreter)
    install(DIRECTORY ${PYTHON3_BDIST}/ DESTINATION ${Python3_SITELIB}
            COMPONENT python3
            PATTERN "*.pyx" EXCLUDE
            PATTERN "*.c" EXCLUDE
            PATTERN "*.cpp" EXCLUDE)

endif ()

# Documentation
option(DOC "Produce documentation" OFF)
if (DOC)
    # Set Directories
    set(DOC_SOUCES ${CMAKE_BINARY_DIR}/doc_sources)
    set(DOC_HTML_DIR ${CMAKE_BINARY_DIR}/doc_html)
    set(DOC_SOUCES_INC ${CMAKE_BINARY_DIR}/doc_sources/includes)

    # Retrieve sources
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/doc/ DESTINATION ${DOC_SOUCES})

    # Set Doxygen Command
    find_package(Doxygen REQUIRED)
    string(REPLACE ";" " " API_PUBLIC_HEADERS_DOC "${API_PUBLIC_HEADERS}")
    add_custom_target(
            doc ALL
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOC_SOUCES}/Doxyfile > /dev/null
            DEPENDS ${API_PUBLIC_HEADERS}
            WORKING_DIRECTORY ${DOC_SOUCES}
            COMMENT "Generating C/C++ API documentation"
            VERBATIM)

    # Build full Sphinx documentation (Require built Python 3 target)
    if (PYTHON3)
        # Configure Doxygen as Sphinx source
        set(DOC_DOXYGEN_XML_DIR ${DOC_SOUCES}/_doxygen)
        set(DOC_DOXYGEN_XML YES)
        set(DOC_DOXYGEN_HTML NO)

        # Configure Sphinx
        configure_file(${DOC_SOUCES}/conf.py ${DOC_SOUCES}/conf.py)

        # Set Sphinx command
        find_program(SPHINX_EXECUTABLE
                NAMES sphinx-build
                DOC "Sphinx Documentation Builder (sphinx-doc.org)")
        add_custom_command(
                TARGET doc POST_BUILD
                COMMAND export LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}:$LD_LIBRARY_PATH &&
                        ${SPHINX_EXECUTABLE} ${DOC_SOUCES} ${DOC_HTML_DIR} -q -j auto
                WORKING_DIRECTORY ${DOC_SOUCES}
                DEPENDS ${API_PUBLIC_HEADERS}
                COMMENT "Generating Full documentation" VERBATIM)

        # Python3 target required to build its API documentation
        add_dependencies(doc python3)

    # Build Only Doxygen (Contain only C/C++ API documentation)
    else()
        # Configure Doxygen as Main documentation
        set(DOC_DOXYGEN_XML NO)
        set(DOC_DOXYGEN_HTML YES)
    endif ()

    # Configure Doxygen
    configure_file(${DOC_SOUCES}/Doxyfile ${DOC_SOUCES}/Doxyfile)

    # Install documentation
    install(DIRECTORY ${DOC_HTML_DIR}/ DESTINATION ${CMAKE_INSTALL_DOCDIR}/html COMPONENT doc)

endif ()

# Packages
set(CPACK_PACKAGE_NAME "libaccelize-drm")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${CPACK_DESCRIPTION_SUMMARY})
set(CPACK_COMPONENT_RUN_DESCRIPTION ${CPACK_DESCRIPTION_SUMMARY})

## libaccelize_drm package

# Get Linux distribution information
find_program(LSB_RELEASE lsb_release)
if (LSB_RELEASE)
    execute_process(COMMAND lsb_release -is
            OUTPUT_VARIABLE DIST_ID
            OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    string(TOLOWER ${DIST_ID} DIST_ID)

    execute_process(COMMAND lsb_release -cs
            OUTPUT_VARIABLE DIST_CODENAME
            OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    string(TOLOWER ${DIST_CODENAME} DIST_CODENAME)

    execute_process(COMMAND lsb_release -rs
            OUTPUT_VARIABLE DIST_RELEASE
            OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    string(TOLOWER ${DIST_RELEASE} DIST_RELEASE)
endif ()

# Copy OS information with packages
if(EXISTS /usr/lib/os-release)
    file(COPY /usr/lib/os-release DESTINATION ${PACKAGE_OUTPUT})
elseif(EXISTS /etc/os-release)
    file(COPY /etc/os-release DESTINATION ${PACKAGE_OUTPUT})
endif()

# RPM Packages
find_program(PACKAGE_MANAGER_YUM yum)
find_program(PACKAGE_MANAGER_DNF dnf)
if (PACKAGE_MANAGER_YUM OR PACKAGE_MANAGER_DNF)
    set(CPACK_GENERATOR "RPM")
    set(CPACK_RPM_COMPONENT_INSTALL ON)

    # Get dist RPM Release dist value (ex: "el7")
    execute_process(COMMAND sh /usr/lib/rpm/redhat/dist.sh
            OUTPUT_VARIABLE CPACK_RPM_PACKAGE_RELEASE_DIST
            OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)

    # Runtime package
    set(CPACK_RPM_RUN_PACKAGE_NAME "${CPACK_PACKAGE_NAME}")
    set(CPACK_RPM_RUN_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}${CPACK_RPM_PACKAGE_RELEASE_DIST}.${CMAKE_SYSTEM_PROCESSOR}.rpm")

    # Dev package
    set(CPACK_RPM_DEV_PACKAGE_NAME "${CPACK_PACKAGE_NAME}-devel")
    set(CPACK_RPM_DEV_FILE_NAME "${CPACK_RPM_DEV_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}${CPACK_RPM_PACKAGE_RELEASE_DIST}.${CMAKE_SYSTEM_PROCESSOR}.rpm")
    set(CPACK_RPM_DEV_PACKAGE_SUMMARY ${CPACK_COMPONENT_DEV_DESCRIPTION})
    set(CPACK_RPM_DEV_PACKAGE_REQUIRES "${CPACK_PACKAGE_NAME}, libcurl-devel, jsoncpp-devel")

    # Python Package
    if (PYTHON3)
        # Package Dependencies (Without Python)
        set(CPACK_RPM_PYTHON3_PACKAGE_REQUIRES "${CPACK_PACKAGE_NAME} >= ${VERSION_MAJOR}.${VERSION_MINOR}")

        # Find Python3 base package
        execute_process(COMMAND rpm -qf ${Python3_EXECUTABLE} --queryformat '%{NAME}'
                OUTPUT_VARIABLE PYTHON3_PACKAGE
                OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)

        if (PYTHON3_PACKAGE MATCHES "is not owned by any package" OR NOT PYTHON3_PACKAGE)
            # Python3 is not from a Package, define package name
            set(PYTHON3_PACKAGE "python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}")
        else()
            # Python3 is from a package, add it as dependency
            set(CPACK_RPM_PYTHON3_PACKAGE_REQUIRES "${CPACK_RPM_PYTHON3_PACKAGE_REQUIRES}, ${PYTHON3_PACKAGE}")
        endif ()

        # Configure package
        set(CPACK_RPM_PYTHON3_PACKAGE_NAME "${PYTHON3_PACKAGE}-accelize-drm")
        set(CPACK_RPM_PYTHON3_FILE_NAME "${CPACK_RPM_PYTHON3_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}${CPACK_RPM_PACKAGE_RELEASE_DIST}.${CMAKE_SYSTEM_PROCESSOR}.rpm")
        set(CPACK_RPM_PYTHON3_PACKAGE_SUMMARY ${CPACK_COMPONENT_PYTHON3_DESCRIPTION})
        set(CPACK_RPM_PYTHON3_PACKAGE_GROUP "Development/Libraries/Python")

    endif ()

    # Other information
    set(CPACK_RPM_CHANGELOG_FILE ${CMAKE_CURRENT_SOURCE_DIR}/CHANGELOG)
    set(CPACK_RPM_PACKAGE_LICENSE "Apache v2.0")
    set(CPACK_RPM_PACKAGE_GROUP "Development/Libraries/C and C++")

    message(STATUS "Packaging : RPM for ${DIST_ID} ${DIST_RELEASE}")
endif ()

# DEB Packages
find_program(PACKAGE_MANAGER_APT apt-get)
if (PACKAGE_MANAGER_APT)
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEB_COMPONENT_INSTALL ON)
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

    # Get Architecture Debian name
    execute_process(COMMAND dpkg --print-architecture
            OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE
            OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)

    # Runtime package
    set(CPACK_DEBIAN_RUN_PACKAGE_NAME ${CPACK_PACKAGE_NAME})
    set(CPACK_DEBIAN_RUN_FILE_NAME "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}.deb")
    set(CPACK_DEBIAN_RUN_PACKAGE_SECTION libs)

    # Dev package
    set(CPACK_DEBIAN_DEV_PACKAGE_NAME "${CPACK_PACKAGE_NAME}-dev")
    set(CPACK_DEBIAN_DEV_FILE_NAME "${CPACK_DEBIAN_DEV_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}.deb")
    set(CPACK_DEBIAN_DEV_PACKAGE_SECTION libdevel)
    set(CPACK_DEBIAN_DEV_PACKAGE_DEPENDS "${CPACK_PACKAGE_NAME}, libcurl4-openssl-dev, libjsoncpp-dev")

    # Python Package
    if (PYTHON3)
        # Configure package
        set(CPACK_DEBIAN_PYTHON3_PACKAGE_NAME "python3-accelize-drm")
        set(CPACK_DEBIAN_PYTHON3_FILE_NAME "${CPACK_DEBIAN_PYTHON3_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}.deb")
        set(CPACK_DEBIAN_PYTHON3_PACKAGE_SECTION libs)
        set(CPACK_DEBIAN_PYTHON3_PACKAGE_DEPENDS "${CPACK_PACKAGE_NAME} (>= ${VERSION_MAJOR}.${VERSION_MINOR}), python3:any (>= 3.4)")
    endif ()

    # Other information
    message(STATUS "Packaging : DEB for ${DIST_ID} ${DIST_CODENAME}")
endif ()

include(CPack)

# Runtime libraries package
cpack_add_component_group(RUN)
cpack_add_component(libraries GROUP RUN)
cpack_add_component(licenses GROUP RUN)

# Dev/Devel package
cpack_add_component_group(DEV)
cpack_add_component(headers GROUP DEV)
cpack_add_component(doc GROUP DEV)

# Python package
if (PYTHON3)
    cpack_add_component_group(PYTHON3)
    cpack_add_component(python3 GROUP PYTHON3)
endif ()

# Tests
option(TESTS "Generates tests files" OFF)
if(TESTS)
    # Copy tests
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/tests/ DESTINATION ${CMAKE_BINARY_DIR}/tests)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/tests/.coveragerc DESTINATION ${CMAKE_BINARY_DIR})
    configure_file(${CMAKE_BINARY_DIR}/tests/conftest.py ${CMAKE_BINARY_DIR}/tests/conftest.py)
endif()
