cmake_minimum_required(VERSION 3.20.0)
project(test_quic VERSION 0.1.0)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
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

set(LSQUIC_TESTS OFF CACHE BOOL "Compile library unit tests" FORCE)
set(protobuf_BUILD_SHARED_LIBS ON CACHE BOOL "Build Shared Libraries" FORCE)
set(protobuf_BUILD_TESTS OFF CACHE BOOL "Build tests" FORCE)
set(build_static_lib ON CACHE BOOL "Build easyloggingpp as a static library" FORCE)

add_subdirectory(third_party/lsquic)
add_subdirectory(third_party/protobuf)
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

add_subdirectory(mqas)

#collect dependeny binary
set(mqas_dependencies_BINARY ${boringssl_BINARY} CACHE STRING "mqas dependencies binary files")

get_target_property(mqas_TARGET_TYPE mqas TYPE)
if(mqas_TARGET_TYPE STREQUAL "SHARED_LIBRARY")
    list(APPEND mqas_dependencies_BINARY "$<TARGET_FILE:mqas>")
	list(APPEND mqas_dependencies_BINARY "$<TARGET_FILE:libuv::uv>")
endif()

get_target_property(libprotobuf_TARGET_TYPE protobuf::libprotobuf TYPE)
if(libprotobuf_TARGET_TYPE STREQUAL "SHARED_LIBRARY")
    list(APPEND mqas_dependencies_BINARY "$<TARGET_FILE:protobuf::libprotobuf>")
	list(APPEND mqas_dependencies_BINARY "$<TARGET_FILE:protobuf::libprotoc>")
	list(APPEND mqas_dependencies_BINARY "$<TARGET_FILE:protobuf::libprotobuf-lite>")
endif()

if(TARGET abseil_dll)
	get_target_property(abseil_dll_TARGET_TYPE abseil_dll TYPE)
	if(abseil_dll_TARGET_TYPE STREQUAL "SHARED_LIBRARY")
		list(APPEND mqas_dependencies_BINARY "$<TARGET_FILE:abseil_dll>")
	endif()
endif()

get_target_property(utf8_range_TARGET_TYPE utf8_range TYPE)
if(utf8_range_TARGET_TYPE STREQUAL "SHARED_LIBRARY")
    list(APPEND mqas_dependencies_BINARY "$<TARGET_FILE:utf8_range>")
endif()

get_target_property(utf8_validity_TARGET_TYPE utf8_validity TYPE)
if(utf8_validity_TARGET_TYPE STREQUAL "SHARED_LIBRARY")
    list(APPEND mqas_dependencies_BINARY "$<TARGET_FILE:utf8_validity>")
endif()
message("mqas_dependencies_BINARY = ${mqas_dependencies_BINARY}")
#=======================================

if(asio_FOUND)
	add_definitions(-DHAS_ASIO)
	add_subdirectory(src/test_ser)
	add_subdirectory(src/test_client)
endif()
if(BUILD_MQAS_EXAMPLES)
	add_subdirectory(src/echo_ser)
	add_subdirectory(src/echo_client)
endif()
if(BUILD_MQAS_EXAMPLES)
	add_subdirectory(examples/recv_test)
	add_subdirectory(examples/send_test)
	add_subdirectory(examples/recv_file)
	add_subdirectory(examples/send_file)
	add_subdirectory(examples/send_datagram)
	add_subdirectory(examples/recv_datagram)
	add_subdirectory(examples/custom_stream_ser)
	add_subdirectory(examples/custom_stream_cli)
	add_subdirectory(examples/pb_stream_ser)
	add_subdirectory(examples/pb_stream_cli)
	add_subdirectory(examples/test_variant_stream_cli)
	add_subdirectory(examples/test_variant_stream_ser)
	if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
		add_subdirectory(examples/test_p2p_cli)
		if(BUILD_P2P_VOICE)
			add_subdirectory(examples/p2p_voice)
		endif()
	endif()
	add_subdirectory(examples/test_p2p_ser)
	add_subdirectory(examples/test_relay_ser)
	add_subdirectory(examples/test_relay_cli)
endif()
