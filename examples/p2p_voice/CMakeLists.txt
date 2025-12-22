aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} CUR_SRC_FILES)
get_filename_component(CURRENT_DIR_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(CUR_TARGET_NAME ${CURRENT_DIR_NAME})
add_executable(${CUR_TARGET_NAME} ${CUR_SRC_FILES})
add_dependencies(${CUR_TARGET_NAME} mqas)


find_package(unofficial-pdcurses CONFIG REQUIRED)
find_package(Opus CONFIG REQUIRED)
find_package(portaudio CONFIG REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SPEEXDSP REQUIRED speexdsp)

target_include_directories(${CUR_TARGET_NAME} PRIVATE
    ${SPEEXDSP_INCLUDE_DIRS}
)

target_link_directories(${CUR_TARGET_NAME} PRIVATE 
    ${SPEEXDSP_LIBRARY_DIRS}
)

target_link_libraries(${CUR_TARGET_NAME} PRIVATE
        mqas
        unofficial::pdcurses::pdcurses
        Opus::opus
        portaudio
        ${SPEEXDSP_LIBRARIES}
        WebRTC::AudioProcessing
)

add_custom_command(TARGET ${CUR_TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND}
        -D TARGET_DIR="$<TARGET_FILE_DIR:${CUR_TARGET_NAME}>"
        -D FILE_LIST="$<TARGET_FILE_DIR:mqas>/dependencies.ini"
        -P "${CMAKE_SOURCE_DIR}/cmake/CopyFiles.cmake"
        COMMENT "Copying mqas dependencies binary files"
)

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