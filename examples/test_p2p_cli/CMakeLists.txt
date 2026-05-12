aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} CUR_SRC_FILES)
get_filename_component(CURRENT_DIR_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(CUR_TARGET_NAME ${CURRENT_DIR_NAME})
add_executable(${CUR_TARGET_NAME} ${CUR_SRC_FILES})
add_dependencies(${CUR_TARGET_NAME} mqas)


find_package(unofficial-pdcurses CONFIG REQUIRED)

target_link_libraries(${CUR_TARGET_NAME} PRIVATE
        mqas
        unofficial::pdcurses::pdcurses
)


add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/test_p2p.pb.cc ${CMAKE_CURRENT_SOURCE_DIR}/test_p2p.pb.h
    COMMAND $<TARGET_FILE:protobuf::protoc> --cpp_out=. test_p2p.proto
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/test_p2p.proto protobuf::protoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating test_p2p proto"
)

add_custom_target(test_p2p_proto DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/test_p2p.pb.cc ${CMAKE_CURRENT_SOURCE_DIR}/test_p2p.pb.h)

add_dependencies(${CUR_TARGET_NAME} test_p2p_proto)

include(fileop)

add_custom_command(TARGET ${CUR_TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND}
        -D TARGET_DIR="$<TARGET_FILE_DIR:${CUR_TARGET_NAME}>"
        -D FILE_LIST="$<TARGET_FILE_DIR:mqas>/dependencies.ini"
        -P "${CMAKE_SOURCE_DIR}/cmake/CopyFiles.cmake"
        COMMENT "Copying mqas dependencies binary files"
)


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

if (NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/conf_p2p.txt")
    add_custom_command(TARGET ${CUR_TARGET_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "conf_p2p.txt"   ${CMAKE_CURRENT_BINARY_DIR}
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