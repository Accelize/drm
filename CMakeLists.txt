# Copyright (C) 2022, Accelize
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

# CONFIGURE VERSION AND COMMON INFORMATION

## Define package version
# Expected format:
# - Stable            : 1.0.0
# - Alpha             : 1.0.0-alpha.1
# - Beta              : 1.0.0-beta.1
# - Release candidate : 1.0.0-rc.1
set(ACCELIZEDRM_VERSION 3.0.0-alpha.1)

## Define package release number (Number of time this version was packaged)
if (NOT CPACK_PACKAGE_RELEASE)
	# Use 1 as default value if not defined with
	# "-DCPACK_PACKAGE_RELEASE=value" command line argument.
	set(CPACK_PACKAGE_RELEASE 1)
endif()

## Note: Python package version is synchronised with ACCELIZEDRM_VERSION

## Configure common information
set(CPACK_PACKAGE_VENDOR "Accelize")
set(CPACK_PACKAGE_CONTACT "https://www.accelize.com/contact")
set(CPACK_PACKAGE_MAINTAINER "accelize.com")
set(CPACK_DESCRIPTION_SUMMARY "Accelize DRM C and C++ Library")
set(CPACK_COMPONENT_DEV_DESCRIPTION "${CPACK_DESCRIPTION_SUMMARY}, headers files.")
set(CPACK_COMPONENT_PYTHON3_DESCRIPTION "Accelize DRM Python Library")
set(CMAKE_PROJECT_HOMEPAGE_URL https://github.com/Accelize/drm)
set(PROJECT_DOCUMENTATION_URL https://tech.accelize.com/documentation/stable)
set(ACCELIZE_HOMEPAGE https://www.accelize.com)
set(ACCELIZE_EMAIL info@accelize.com)

## Find package CommitID
set(ACCELIZEDRM_LONG_VERSION ${ACCELIZEDRM_VERSION})
find_program(GIT git)
if (GIT)
	execute_process(COMMAND git rev-parse --short HEAD
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		OUTPUT_VARIABLE GIT_COMMIT
		OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
	)
	if (GIT_COMMIT)
		# Add as Semantic versioning build metadata
		set(ACCELIZEDRM_LONG_VERSION ${ACCELIZEDRM_VERSION}+${GIT_COMMIT})
	endif ()
endif ()

## Find ABI version
string(REPLACE "." ";" VERSION_LIST ${ACCELIZEDRM_VERSION})
list(GET VERSION_LIST 0 VERSION_MAJOR)
list(GET VERSION_LIST 1 VERSION_MINOR)
list(GET VERSION_LIST 2 VERSION_BUG)
set(ABI_VERSION ${VERSION_MAJOR})

## Show version information
message(STATUS "Library version : ${ACCELIZEDRM_VERSION}")
message(STATUS "Library long version : ${ACCELIZEDRM_LONG_VERSION}")
message(STATUS "Library Package version : ${ACCELIZEDRM_VERSION}-${CPACK_PACKAGE_RELEASE}")

# CONFIGURE BUILD
project(accelize/drm)
option(PKG "Produce packages" OFF)

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
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(-fvisibility=hidden)
if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
	# Enable warnings
	add_compile_options(-Wall -Wextra -Wnon-virtual-dtor -Wpedantic -Werror=format-security)

	# Optimize for Debugging
	if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
		add_compile_options(-fdebug-prefix-map=${CMAKE_SOURCE_DIR}=.)
	endif()

	# ABI check
	add_compile_options(-g -Og)

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

add_definitions(-DBUILDING_DRMLIB)
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")

## Build requirements
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

## Build JsonCpp library
set(JSONCPP_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp)
set(JSONCPP_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/jsoncpp)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC" )

include(ExternalProject)
ExternalProject_Add(jsoncpp_project
	SOURCE_DIR ${JSONCPP_SOURCE_DIR}
	BINARY_DIR ${JSONCPP_BINARY_DIR}
	PREFIX ${JSONCPP_BINARY_DIR}
	CMAKE_ARGS "-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS} "
		   "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} "
		   "-DBUILD_OBJECT_LIBS=OFF "
		   "-DBUILD_STATIC_LIBS=ON "
		   "-DBUILD_SHARED_LIBS=OFF "
		   "-DJSONCPP_WITH_TESTS=OFF "
		   "-DJSONCPP_WITH_EXAMPLE=OFF "
	INSTALL_COMMAND ""
)
ExternalProject_Get_Property(jsoncpp_project BINARY_DIR)
set(JSONCPP_LIBRARYDIR ${BINARY_DIR}/lib/)
add_library(jsoncpp_lib STATIC IMPORTED)
set_property(TARGET jsoncpp_lib PROPERTY IMPORTED_LOCATION ${JSONCPP_LIBRARYDIR}/libjsoncpp.a)
add_dependencies(jsoncpp_lib jsoncpp_project)

set(JSONCPP_INCLUDEDIR ${JSONCPP_SOURCE_DIR}/include)
include_directories(${JSONCPP_INCLUDEDIR})

## Curl library
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
	drm_controller_sdk/source/HAL/DrmControllerRegistersStrategy_v3_1_0.cpp
	drm_controller_sdk/source/HAL/DrmControllerRegistersStrategy_v3_2_0.cpp
	drm_controller_sdk/source/HAL/DrmControllerRegistersStrategy_v3_2_1.cpp
	drm_controller_sdk/source/HAL/DrmControllerRegistersStrategy_v3_2_2.cpp
	drm_controller_sdk/source/HAL/DrmControllerRegistersStrategy_v4_0_0.cpp
	drm_controller_sdk/source/HAL/DrmControllerRegistersStrategy_v4_0_1.cpp
	drm_controller_sdk/source/HAL/DrmControllerRegistersStrategy_v4_1_0.cpp
	drm_controller_sdk/source/HAL/DrmControllerRegistersStrategy_v4_2_0.cpp
	drm_controller_sdk/source/HAL/DrmControllerRegistersStrategy_v4_2_1.cpp
	drm_controller_sdk/source/HAL/DrmControllerRegistersStrategy_v6_0_0.cpp
	drm_controller_sdk/source/HAL/DrmControllerRegistersStrategy_v6_0_1.cpp
	drm_controller_sdk/source/HAL/DrmControllerRegistersStrategy_v7_0_0.cpp
	drm_controller_sdk/source/HAL/DrmControllerRegistersStrategy_v7_1_0.cpp
	drm_controller_sdk/source/HAL/DrmControllerRegistersStrategy_v8_0_0.cpp
	drm_controller_sdk/source/HAL/DrmControllerRegistersStrategy_v8_1_0.cpp
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
    include/accelize/drm/drm_manager.h
    include/accelize/drm/error.h
    include/accelize/drm.h
    include/accelize/drmc/common.h
    include/accelize/drmc/errorcode.h
    include/accelize/drmc/wrapper.h
    include/accelize/drmc.h
    include/pnr/provencore.h
)

set(API_PUBLIC_HEADERS)
foreach(_header_file ${API_HEADERS})
  get_filename_component(_header_file_path ${_header_file} DIRECTORY)
  add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/${_header_file}
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

## Sources
set(TARGET_SOURCES
    source/ws_client.cpp
    source/csp.cpp
    source/drm_manager.cpp
    source/utils.cpp
    source/error.cpp
    source/log.cpp
    source/provencore.cpp
)
add_library(accelize_drm SHARED ${TARGET_SOURCES})
target_include_directories(accelize_drm PRIVATE spdlog/include)

## libaccelize_drm
set_target_properties(accelize_drm PROPERTIES VERSION ${ACCELIZEDRM_VERSION} SOVERSION ${ABI_VERSION})
target_link_libraries(accelize_drm ${CURL_LIBRARIES})
target_link_libraries(accelize_drm jsoncpp_lib)
target_link_libraries(accelize_drm drm_controller_lib)
target_link_libraries(accelize_drm ${CMAKE_THREAD_LIBS_INIT})

## libaccelize_drmc
set(TARGET_C_SOURCES
    source/c/wrapperc.cpp
    source/log.cpp)

add_library(accelize_drmc SHARED ${TARGET_C_SOURCES})
target_include_directories(accelize_drmc PRIVATE spdlog/include)
set_target_properties(accelize_drmc PROPERTIES VERSION ${ACCELIZEDRM_VERSION} SOVERSION ${ABI_VERSION})
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
	find_package (Python3 COMPONENTS Interpreter Development)

	if (NOT EXISTS "${Python3_SITEARCH}")
		# Fix bad Python3_SITEARCH in some cases (Example Ubuntu)
		execute_process(
			COMMAND "${Python3_EXECUTABLE}" -c "import sys;print(sys.path[-1])"
			OUTPUT_VARIABLE Python3_SITEARCH
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)
	endif()

	## Retrieves sources
	set(PYTHON3_BDIST_RELATIVE python3_bdist)
	set(PYTHON3_BDIST ${CMAKE_BINARY_DIR}/${PYTHON3_BDIST_RELATIVE})
	set(PYTHON_SOURCE ${CMAKE_BINARY_DIR}/python_src)
	set(PYTHON_SETUP ${PYTHON_SOURCE}/setup.py)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/python/ DESTINATION ${PYTHON_SOURCE}
		FILES_MATCHING PATTERN "*.in" PATTERN "*.py*" PATTERN "*.pxd"
		PATTERN "*.md" PATTERN "*.service")
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
						  build_ext -I ${CMAKE_INCLUDE_DIR}:${JSONCPP_INCLUDEDIR} -L ${CMAKE_BINARY_DIR} ${BUILD_EXT_EXTRA} -j ${CPU_COUNT}
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
		COMMENT "Building accelize_drm Python 3 (With wheel Package)"
		DEPENDS ${API_PUBLIC_HEADERS}
		VERBATIM
	)
	if (PKG)
		add_custom_command(
			TARGET python3 POST_BUILD
			COMMAND ${Python3_EXECUTABLE} ${PYTHON_SETUP} sdist -d ${PACKAGE_OUTPUT}
			WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
			COMMENT "Building accelize_drm Python 3 source Package"
			VERBATIM
		)
	endif ()

	add_dependencies(python3 accelize_drm)
	add_dependencies(python3 accelize_drmc)
	add_dependencies(python3 Python3::Interpreter)
	install(DIRECTORY ${PYTHON3_BDIST}/ DESTINATION ${Python3_SITEARCH}
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
	message( "DOXYGEN_EXECUTABLE=${DOXYGEN_EXECUTABLE}" )
	message( "DOC_SOUCES=${DOC_SOUCES}" )

	# Build full Sphinx documentation (Require built Python 3 target)
	if (PYTHON3)
		# Configure Doxygen as Sphinx source
		set(DOC_DOXYGEN_XML_DIR ${DOC_SOUCES}/_doxygen)
		set(DOC_DOXYGEN_XML YES)
		set(DOC_DOXYGEN_HTML NO)
		message( "DOC_DOXYGEN_XML_DIR=${DOC_DOXYGEN_XML_DIR}" )
		message( "DOC_DOXYGEN_XML=${DOC_DOXYGEN_XML}" )
		message( "DOC_DOXYGEN_HTML=${DOC_DOXYGEN_HTML}" )

		# Configure Sphinx
		configure_file(${DOC_SOUCES}/conf.py ${DOC_SOUCES}/conf.py)

		# Set Sphinx command
		find_program(SPHINX_EXECUTABLE
			NAMES sphinx-build
			DOC "Sphinx Documentation Builder (sphinx-doc.org)"
		)
		message( "DOC_SOUCES=${DOC_SOUCES}" )
		message( "DOC_HTML_DIR=${DOC_HTML_DIR}" )
		message( "SPHINX_EXECUTABLE=${SPHINX_EXECUTABLE}" )
		message( "WORKING_DIRECTORY=${WORKING_DIRECTORY}" )
		message( "CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}" )
		add_custom_command(
			TARGET doc POST_BUILD
			COMMAND export LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}:$LD_LIBRARY_PATH &&
				${SPHINX_EXECUTABLE} ${DOC_SOUCES} ${DOC_HTML_DIR} -q -j auto
			WORKING_DIRECTORY ${DOC_SOUCES}
			DEPENDS ${API_PUBLIC_HEADERS}
			COMMENT "Generating Full documentation" VERBATIM
		)

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

endif ()

# Packages
if (PKG)
	set(CPACK_PACKAGE_VERSION ${ACCELIZEDRM_VERSION})
	set(CPACK_PACKAGE_NAME "libaccelize-drm")
	set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${CPACK_DESCRIPTION_SUMMARY})
	set(CPACK_COMPONENT_RUN_DESCRIPTION ${CPACK_DESCRIPTION_SUMMARY})

	## libaccelize_drm package

	# Get Linux distribution information from os-release
	if (EXISTS "/etc/os-release")
		set(OS_RELEASE "/etc/os-release")
	elseif(EXISTS "/usr/lib/os-release")
		set(OS_RELEASE "/usr/lib/os-release")
	endif()

	if (OS_RELEASE)
		# Get information from file
		file (STRINGS ${OS_RELEASE} lines)
		foreach (line ${lines})
			# Get Distribution name
			if (line MATCHES "^ID=(.*)")
				SET (DIST_ID "${CMAKE_MATCH_1}")
				if (DIST_ID MATCHES "^['\"](.*)['\"]\$")
					set (DIST_ID "${CMAKE_MATCH_1}")
				endif ()
			endif ()
			# Get distribution codename
			if (line MATCHES "^VERSION_CODENAME=(.*)")
				SET (DIST_CODENAME "${CMAKE_MATCH_1}")
				if (DIST_CODENAME MATCHES "^['\"](.*)['\"]\$")
					set (DIST_CODENAME "${CMAKE_MATCH_1}")
				endif ()
			endif ()
			# Get distribution full version string
			if (line MATCHES "^VERSION=(.*)")
				SET (DIST_VERSION "${CMAKE_MATCH_1}")
				if (DIST_VERSION MATCHES "^['\"](.*)['\"]\$")
					set (DIST_VERSION "${CMAKE_MATCH_1}")
				endif ()
			endif ()
			# Get distribution version number
			if (line MATCHES "^VERSION_ID=(.*)")
				SET (VERSION_ID "${CMAKE_MATCH_1}")
				if (VERSION_ID MATCHES "^['\"](.*)['\"]\$")
					set (VERSION_ID "${CMAKE_MATCH_1}")
				endif ()
			endif ()
		endforeach ()
		if (NOT DIST_CODENAME AND DIST_VERSION)
			# If VERSION_CODENAME is missing, get codename from full version
			if (DIST_VERSION MATCHES "^.* [\(](.*)[\)]\$")
				set (DIST_CODENAME "${CMAKE_MATCH_1}")
			endif ()
		endif()
	endif ()

	# RPM Packages
	find_program(RPMBUILD rpmbuild)
	if (RPMBUILD)
		set(CPACK_GENERATOR "RPM")
		set(CPACK_RPM_COMPONENT_INSTALL ON)
	string(REPLACE "-" "_" CPACK_RPM_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION})

		# Get dist RPM Release dist value (ex: "el7")
		execute_process(COMMAND sh /usr/lib/rpm/redhat/dist.sh
			OUTPUT_VARIABLE CPACK_RPM_PACKAGE_RELEASE_DIST
			OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
		)

		# Runtime package
		set(CPACK_RPM_RUN_PACKAGE_NAME "${CPACK_PACKAGE_NAME}")
		set(CPACK_RPM_RUN_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_RPM_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}${CPACK_RPM_PACKAGE_RELEASE_DIST}.${CMAKE_SYSTEM_PROCESSOR}.rpm")

		# Dev package
		set(CPACK_RPM_DEV_PACKAGE_NAME "${CPACK_PACKAGE_NAME}-devel")
		set(CPACK_RPM_DEV_PACKAGE_ARCHITECTURE "noarch")
		set(CPACK_RPM_DEV_FILE_NAME "${CPACK_RPM_DEV_PACKAGE_NAME}-${CPACK_RPM_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}${CPACK_RPM_PACKAGE_RELEASE_DIST}.noarch.rpm")
		set(CPACK_RPM_DEV_PACKAGE_SUMMARY ${CPACK_COMPONENT_DEV_DESCRIPTION})
		set(CPACK_RPM_DEV_PACKAGE_REQUIRES "${CPACK_PACKAGE_NAME}, libcurl-devel")

		# Python Package
		if (PYTHON3)
			# Package Dependencies (Without Python)
			set(CPACK_RPM_PYTHON3_PACKAGE_REQUIRES "${CPACK_PACKAGE_NAME} >= ${VERSION_MAJOR}.${VERSION_MINOR}")

			# RHEL/CentOS 7 package was renamed from "python36"
			if (VERSION_ID MATCHES "7")
				set(CPACK_RPM_PACKAGE_OBSOLETES "python36-accelize-drm")
			endif ()

			# Configure package
			set(CPACK_RPM_PYTHON3_PACKAGE_NAME "python3-accelize-drm")
			set(CPACK_RPM_PYTHON3_FILE_NAME "${CPACK_RPM_PYTHON3_PACKAGE_NAME}-${CPACK_RPM_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}${CPACK_RPM_PACKAGE_RELEASE_DIST}.${CMAKE_SYSTEM_PROCESSOR}.rpm")
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
	find_program(DPKGBUILD dpkg-buildpackage)
	if (DPKGBUILD)
		set(CPACK_PACKAGE_VERSION "${ACCELIZEDRM_VERSION}-${CPACK_PACKAGE_RELEASE}+${DIST_CODENAME}")
		set(CPACK_GENERATOR "DEB")
		set(CPACK_DEB_COMPONENT_INSTALL ON)
		set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

		# Get Architecture Debian name
		execute_process(COMMAND dpkg --print-architecture
			OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE
			OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
		)

		# Runtime package
		set(CPACK_DEBIAN_RUN_PACKAGE_NAME ${CPACK_PACKAGE_NAME})
		set(CPACK_DEBIAN_RUN_FILE_NAME "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}.deb")
		set(CPACK_DEBIAN_RUN_PACKAGE_SECTION libs)

		# Dev package
		set(CPACK_DEBIAN_DEV_PACKAGE_NAME "${CPACK_PACKAGE_NAME}-dev")
		set(CPACK_DEBIAN_DEV_FILE_NAME "${CPACK_DEBIAN_DEV_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}_all.deb")
		set(CPACK_DEBIAN_DEV_PACKAGE_SECTION libdevel)
		set(CPACK_DEBIAN_DEV_PACKAGE_ARCHITECTURE "all")
		set(CPACK_DEBIAN_DEV_PACKAGE_DEPENDS "${CPACK_PACKAGE_NAME}, libcurl4-openssl-dev")

		# Python Package
		if (PYTHON3)
			# Configure package
			set(CPACK_DEBIAN_PYTHON3_PACKAGE_NAME "python3-accelize-drm")
			set(CPACK_DEBIAN_PYTHON3_FILE_NAME "${CPACK_DEBIAN_PYTHON3_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}.deb")
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

	# Python package
	if (PYTHON3)
		cpack_add_component_group(PYTHON3)
		cpack_add_component(python3 GROUP PYTHON3)
	endif ()
endif ()

# Tests
option(TESTS "Generates tests files" OFF)
option(AWS "Generates specific tests on AWS" OFF)
if(TESTS)
	# Copy tests
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/tests/ DESTINATION ${CMAKE_BINARY_DIR}/tests
		PATTERN __pycache__ EXCLUDE PATTERN .pyc EXCLUDE)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/tests/.coveragerc DESTINATION ${CMAKE_BINARY_DIR})
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/pytest.ini DESTINATION ${CMAKE_BINARY_DIR})
	configure_file(${CMAKE_BINARY_DIR}/tests/conftest.py ${CMAKE_BINARY_DIR}/tests/conftest.py)

	if (AWS)
		# Compile unittest.cpp application
		if ( NOT DEFINED ENV{SDK_DIR} )
			if ( ${CMAKE_BUILD_TYPE} STREQUAL "Debug" )
				message( SEND_ERROR "SDK_DIR environment variable is not defined: "
					"Cannot compile tests with AWS FPGA Management driver." )
			endif()
		else()
			find_path(FPGAMGMTLIB_INCLUDE_DIRS
				NAMES fpga_mgmt.h
				PATHS $ENV{SDK_DIR}/userspace/include /usr/local/include NO_DEFAULT_PATH
			)
			message( "FPGAMGMTLIB_INCLUDE_DIRS=${FPGAMGMTLIB_INCLUDE_DIRS}" )
			find_library(FPGAMGMTLIB_LIBRARIES
				NAMES fpga_mgmt
				PATHS $ENV{SDK_DIR}/userspace/lib/so /usr/local/lib64 NO_DEFAULT_PATH
			)
			message( "FPGAMGMTLIB_LIBRARIES=${FPGAMGMTLIB_LIBRARIES}" )
			add_executable( unittests ${CMAKE_CURRENT_SOURCE_DIR}/tests/unittests.cpp )
			set_target_properties( unittests
				PROPERTIES
			    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
			)
			target_include_directories( unittests PUBLIC ${FPGAMGMTLIB_INCLUDE_DIRS} )
			target_include_directories( unittests PUBLIC ${JSONCPP_INCLUDEDIR} )
			target_link_libraries( unittests accelize_drmc )
			target_link_libraries( unittests ${FPGAMGMTLIB_LIBRARIES} )
			target_link_libraries( unittests jsoncpp_lib )
		endif()
	endif()
endif()

# uninstall target
if(NOT TARGET uninstall)
	configure_file(
		"${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
		"${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
		IMMEDIATE @ONLY
	)

	add_custom_target(uninstall
		COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
	)
endif()
