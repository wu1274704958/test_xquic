aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} CUR_SRC_FILES)
set(CUR_TARGET_NAME recv_datagram)
add_executable(${CUR_TARGET_NAME} ${CUR_SRC_FILES})
add_dependencies(${CUR_TARGET_NAME} mqas)
target_link_libraries(${CUR_TARGET_NAME} PRIVATE 
    mqas
)

add_custom_command(TARGET ${CUR_TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND}
        -D TARGET_DIR="$<TARGET_FILE_DIR:${CUR_TARGET_NAME}>"
        -D FILE_LIST="$<TARGET_FILE_DIR:mqas>/dependencies.ini"
        -P "${CMAKE_SOURCE_DIR}/cmake/CopyFiles.cmake"
        COMMENT "Copying mqas dependencies binary files"
)

if (NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/cert-def.pem")
#openssl req -newkey rsa:4096 -nodes -sha256 -keyout key-def.pem -x509 -days 365 -out cert-def.pem
    add_custom_command(TARGET ${CUR_TARGET_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "certificate/cert-def.pem"   ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "certificate/key-def.pem"   ${CMAKE_CURRENT_BINARY_DIR}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )

endif()

if (NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/conf.txt")
    add_custom_command(TARGET ${CUR_TARGET_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "conf.txt"   ${CMAKE_CURRENT_BINARY_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
endif()