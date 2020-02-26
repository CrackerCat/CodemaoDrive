cmake_minimum_required(VERSION 2.8.7)
project(CodemaoDrive)

if (MSVC_VERSION GREATER_EQUAL "1900")
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("/std:c++latest" _cpp_latest_flag_supported)
    if (_cpp_latest_flag_supported)
        add_compile_options("/std:c++latest")
    endif()
endif()

set(BUILD_CPR_TESTS OFF CACHE BOOL "Don't build cpr tests")

add_subdirectory(opt/cpr)

include_directories(opt/rapidjson/include)
include_directories(opt/argparse/include)
include_directories(opt/)
include_directories(${CPR_INCLUDE_DIRS})

add_executable(CodemaoDrive src/Main.cpp)

target_link_libraries(CodemaoDrive ${CPR_LIBRARIES})