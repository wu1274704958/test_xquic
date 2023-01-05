cmake_minimum_required(VERSION 3.20.0)
project(test_quic VERSION 0.1.0)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
if(CMAKE_CXX_COMPILER_ID MATCHES MSVC)
  add_compile_definitions("/permissive-")
  add_definitions(-DWIN32_LEAN_AND_MEAN)
  set(OTH_LIBRARY ws2_32.lib)
endif()

set(CMAKE_CXX_STANDARD 20)

set(COMM_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

find_package(ZLIB REQUIRED)
find_package(boringssl REQUIRED)
find_package(lsquic CONFIG REQUIRED)
add_subdirectory(src/test_ser)
