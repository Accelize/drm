cmake_minimum_required(VERSION 3.12)

project (drmutil)

# Path to custom modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(-Wall -Wextra -pedantic -Wno-variadic-macros -Wnon-virtual-dtor -Werror=format-security)
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

find_package(FPGAMGMTLIB REQUIRED)
find_package(DRMLIB REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP jsoncpp)
include_directories(${JSONCPP_INCLUDEDIR})

# demo C
add_executable(drmutil_c drmutil.c)
target_include_directories(drmutil_c PUBLIC ${FPGAMGMTLIB_INCLUDE_DIRS})
target_include_directories(drmutil_c PRIVATE ${DRMLIB_INCLUDE_DIRS})
target_link_libraries(drmutil_c ${DRMLIB_C_LIBRARIES})
target_link_libraries(drmutil_c pthread)
target_link_libraries(drmutil_c ${FPGAMGMTLIB_LIBRARIES})

# demo CPP
add_executable(drmutil_cpp drmutil.cpp)
target_include_directories(drmutil_cpp PUBLIC ${FPGAMGMTLIB_INCLUDE_DIRS})
target_include_directories(drmutil_cpp PRIVATE ${DRMLIB_INCLUDE_DIRS})
target_link_libraries(drmutil_cpp ${DRMLIB_LIBRARIES})
target_link_libraries(drmutil_cpp pthread)
target_link_libraries(drmutil_cpp ${FPGAMGMTLIB_LIBRARIES})
