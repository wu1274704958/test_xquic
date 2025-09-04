cmake_minimum_required(VERSION 3.20.0)
project(test_quic VERSION 0.1.0)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
if(CMAKE_CXX_COMPILER_ID MATCHES MSVC)
  add_compile_options("/permissive-")
endif()

add_definitions(-DELPP_THREAD_SAFE)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	add_definitions(-DWIN32_LEAN_AND_MEAN -DWIN32 -DNOMINMAX)
	add_compile_options(/EHsc)
	set(OTH_LIBRARY ws2_32.lib)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
	add_definitions(-DPF_ANDROID)
	set(OTH_LIBRARY "android" "log")
endif()
message("CMAKE_SYSTEM_NAME = ${CMAKE_SYSTEM_NAME}")
set(CMAKE_CXX_STANDARD 20)

find_package(ZLIB REQUIRED)
find_package(boringssl REQUIRED)
#find_package(lsquic CONFIG REQUIRED)
find_package(asio CONFIG)
find_package(libuv CONFIG REQUIRED)
find_package(toml11 CONFIG REQUIRED)
#find_package(protobuf CONFIG REQUIRED)
find_package(sigc++-3 CONFIG REQUIRED)
find_package(Boost REQUIRED)

set(SSLLIB_INCLUDE ${boringssl_INCLUDE_DIR})
set(LIBSSL_LIB_ssl ${boringssl_ssl_LIBRARY}) 
set(LIBSSL_LIB_crypto ${boringssl_crypto_LIBRARY})

add_subdirectory(third_party/lsquic)

set(protobuf_BUILD_SHARED_LIBS ON FORCE)

add_subdirectory(third_party/protobuf)

set(build_static_lib ON FORCE)

add_subdirectory(third_party/easyloggingpp)

add_library(lsquic::lsquic OBJECT IMPORTED GLOBAL)

set(lsquic_include_dirs 
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/lsquic/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/lsquic/wincompat"
)

set_target_properties(lsquic::lsquic PROPERTIES 
	INTERFACE_INCLUDE_DIRECTORIES "${lsquic_include_dirs}"
)

set(easyloggingpp_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/easyloggingpp/src")
set(protobuf_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/protobuf/src")

set(COMM_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include" ${Boost_INCLUDE_DIRS})

if(BUILD_MQAS_EXAMPLES AND BUILD_P2P_VOICE)
	add_subdirectory(third_party/webrtc-audio-processing)
endif()

if(asio_FOUND)
	add_definitions(-DHAS_ASIO)
	add_subdirectory(src/test_ser)
	add_subdirectory(src/test_client)
endif()
if(BUILD_MQAS_EXAMPLES)
	add_subdirectory(src/echo_ser)
	add_subdirectory(src/echo_client)
endif()
add_subdirectory(mqas)
if(BUILD_MQAS_EXAMPLES)
	add_subdirectory(examples/recv_test)
endif()
