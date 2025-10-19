cmake_minimum_required(VERSION 3.26)
project(proj1 CXX)

# global compiler settings
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# option to build tests
option(BUILD_TESTS "Build test executables" ON)

# include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# library sources
file(GLOB LIB_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/lib/*.cpp")

# executable: main program
set(CONFIG_FILE_PATH "${CMAKE_SOURCE_DIR}/ds/config.txt")
add_definitions(-DCONFIG_FILE_PATH="${CONFIG_FILE_PATH}")

# copy config.txt into build directory
configure_file(
    ${CMAKE_SOURCE_DIR}/ds/config.txt
    ${CMAKE_BINARY_DIR}/ds/config.txt
    COPYONLY
)

# add executable before linking libraries
add_executable(proj1
    ds/main.cpp
    ${LIB_SOURCES}
)

# link SCTP library after defining the executable
target_link_libraries(proj1 PRIVATE sctp)

# tests (conditionally included)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
