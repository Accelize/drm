cmake_minimum_required(VERSION 3.12)

project (demo)

# Path to custom modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wnon-virtual-dtor")
add_compile_options(-Wall -Wextra -pedantic)
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

find_package(FPGAMGMTLIB REQUIRED)
find_package(DRMLIB REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP jsoncpp)
include_directories(${JSONCPP_INCLUDEDIR})

# demo C
add_executable(demo_c demo.c)
target_include_directories(demo_c PUBLIC ${FPGAMGMTLIB_INCLUDE_DIRS})
target_include_directories(demo_c PRIVATE ${DRMLIB_INCLUDE_DIRS})
target_link_libraries(demo_c ${DRMLIB_C_LIBRARIES})
target_link_libraries(demo_c pthread)
target_link_libraries(demo_c ${FPGAMGMTLIB_LIBRARIES})

# demo CPP
add_executable(demo_cpp demo.cpp)
target_include_directories(demo_cpp PUBLIC ${FPGAMGMTLIB_INCLUDE_DIRS})
target_include_directories(demo_cpp PRIVATE ${DRMLIB_INCLUDE_DIRS})
target_link_libraries(demo_cpp ${DRMLIB_LIBRARIES})
target_link_libraries(demo_cpp pthread)
target_link_libraries(demo_cpp ${FPGAMGMTLIB_LIBRARIES})
