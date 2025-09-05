aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} CUR_SRC_FILES)
get_filename_component(CURRENT_DIR_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(CUR_TARGET_NAME ${CURRENT_DIR_NAME})
add_executable(${CUR_TARGET_NAME} ${CUR_SRC_FILES})
add_dependencies(${CUR_TARGET_NAME} mqas easyloggingpp)


find_package(unofficial-pdcurses CONFIG REQUIRED)
find_package(Opus CONFIG REQUIRED)
find_package(portaudio CONFIG REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SPEEXDSP REQUIRED speexdsp)

target_include_directories(${CUR_TARGET_NAME} PRIVATE 
    ${boringssl_INCLUDE_DIR}
    ${SPEEXDSP_INCLUDE_DIRS}
    ${easyloggingpp_INCLUDE_DIR}
)

target_link_directories(${CUR_TARGET_NAME} PRIVATE 
    ${SPEEXDSP_LIBRARY_DIRS}
)

target_link_libraries(${CUR_TARGET_NAME} PRIVATE
        lsquic::lsquic
        lsquic
        ${OTH_LIBRARY}
        ${boringssl_LIBRARY}
        ZLIB::ZLIB
        $<IF:$<TARGET_EXISTS:libuv::uv_a>,libuv::uv_a,libuv::uv>
        mqas
        easyloggingpp
        protobuf::libprotoc protobuf::libprotobuf protobuf::libprotobuf-lite
        sigc-3.0
        unofficial::pdcurses::pdcurses
        Opus::opus
        portaudio
        ${SPEEXDSP_LIBRARIES}
        WebRTC::AudioProcessing
)

include(fileop)

bulk_copy_files(${CUR_TARGET_NAME} ${CMAKE_CURRENT_BINARY_DIR} POST_BUILD mqas_dependencies_BINARY)

get_target_property(WebRTC_AudioProcessing_BINARY WebRTC::AudioProcessing IMPORTED_LOCATION)

bulk_copy_files(${CUR_TARGET_NAME} ${CMAKE_CURRENT_BINARY_DIR} POST_BUILD WebRTC_AudioProcessing_BINARY)

if (NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/conf.txt")
    add_custom_command(TARGET ${CUR_TARGET_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "conf.txt"   ${CMAKE_CURRENT_BINARY_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
endif()

if (NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/conf_relay.txt")
    add_custom_command(TARGET ${CUR_TARGET_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "conf_relay.txt"   ${CMAKE_CURRENT_BINARY_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
endif()

if (NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/audio_log_conf.txt")
    add_custom_command(TARGET ${CUR_TARGET_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "audio_log_conf.txt"   ${CMAKE_CURRENT_BINARY_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
endif()

if (NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/cert-def.pem")
#openssl req -newkey rsa:4096 -nodes -sha256 -keyout key-def.pem -x509 -days 365 -out cert-def.pem
    add_custom_command(TARGET ${CUR_TARGET_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "certificate/cert-def.pem"   ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "certificate/key-def.pem"   ${CMAKE_CURRENT_BINARY_DIR}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )

endif()